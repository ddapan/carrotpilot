#ifndef QCOM2

#include "system/camerad/cameras/camera_common.h"
#include "system/camerad/cameras/mipi/camera_mipi.h"
#include "system/camerad/cameras/usb/camera_usb.h"
#include "common/timing.h"
#include "common/util.h"
#include "cereal/messaging/messaging.h"

#include <cassert>
#include <iostream>
#include <cstring>
#include <errno.h>
#include <signal.h>
#include <atomic>

// 全局退出标志
std::atomic<bool> g_should_exit(false);

// 信号处理函数
void signal_handler(int sig) {
  if (sig == SIGINT) {
    std::cout << "\nReceived Ctrl+C, shutting down..." << std::endl;
    g_should_exit = true;
  }
}

void camerad_thread() {
  // 设置信号处理
  signal(SIGINT, signal_handler);

  // 检查debug模式
  const char *debug_env = getenv("CAMERAD_DEBUG");
  bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);

  if (debug_mode) {
    std::cout << "[DEBUG] Camerad debug mode enabled" << std::endl;
  }

  // 检查是否使用USB摄像头
  const char *use_webcam_env = getenv("USE_WEBCAM");
  bool use_webcam = use_webcam_env && (strcmp(use_webcam_env, "1") == 0 || strcmp(use_webcam_env, "true") == 0);

  std::cout << "[DEBUG] Use USB camera: " << (use_webcam ? "yes" : "no") << std::endl;

  // 从环境变量获取配置
  const char *road_cam_path = getenv("ROAD_CAM_PATH");
  const char *road_cam = getenv("ROAD_CAM");
  const char *road_cam_width = getenv("ROAD_CAM_WIDTH");
  const char *road_cam_height = getenv("ROAD_CAM_HEIGHT");
  const char *road_cam_framerate = getenv("ROAD_CAM_FRAMERATE");

  // 设备路径优先级：ROAD_CAM_PATH > ROAD_CAM > 默认
  const char *device_path;
  if (road_cam_path) {
    device_path = road_cam_path;
    if (debug_mode) {
      std::cout << "[DEBUG] Using ROAD_CAM_PATH: " << device_path << std::endl;
    }
  } else if (road_cam) {
    static char cam_path[64];
    snprintf(cam_path, sizeof(cam_path), "/dev/video%s", road_cam);
    device_path = cam_path;
    if (debug_mode) {
      std::cout << "[DEBUG] Using ROAD_CAM: " << road_cam << " -> " << device_path << std::endl;
    }
  } else {
    if (use_webcam) {
      device_path = "/dev/video45"; // USB摄像头默认路径
    } else {
      device_path = "/dev/video0"; // MIPI摄像头默认路径
    }
    if (debug_mode) {
      std::cout << "[DEBUG] Using default device path: " << device_path << std::endl;
    }
  }

  // 分辨率设置
  int width = road_cam_width ? atoi(road_cam_width) : 1920;
  int height = road_cam_height ? atoi(road_cam_height) : 1080;

  if (debug_mode) {
    std::cout << "[DEBUG] Resolution: " << width << "x" << height << std::endl;
  }

  // 帧率设置
  int framerate = road_cam_framerate ? atoi(road_cam_framerate) : 20;

  if (debug_mode) {
    std::cout << "[DEBUG] Framerate: " << framerate << " FPS" << std::endl;
  }

  // 实际使用的分辨率（用于自适应处理）
  int actual_width = width;
  int actual_height = height;

  // 初始化摄像头
  CameraMipi mipi_camera;
  CameraUSB usb_camera;

  int ret = 0;
  if (use_webcam) {
    // 初始化USB摄像头
    ret = usb_camera.init(device_path, width, height, framerate);
    if (ret < 0) {
      std::cerr << "Failed to initialize USB camera: " << strerror(errno) << std::endl;
      std::cerr << "Device path: " << device_path << std::endl;
      return;
    }
    std::cout << "Successfully initialized USB camera: " << device_path << std::endl;

    // 获取实际分辨率
    actual_width = usb_camera.get_actual_width();
    actual_height = usb_camera.get_actual_height();
    std::cout << "[DEBUG] Actual USB camera resolution: " << actual_width << "x" << actual_height << std::endl;

    // 启动摄像头
    ret = usb_camera.start();
    if (ret < 0) {
      std::cerr << "Failed to start USB camera" << std::endl;
      return;
    }


  } else {
    // 初始化MIPI摄像头
    ret = mipi_camera.init(device_path, width, height, framerate);
    if (ret < 0) {
      std::cerr << "Failed to initialize MIPI camera: " << strerror(errno) << std::endl;
      std::cerr << "Device path: " << device_path << std::endl;
      return;
    }
    std::cout << "Successfully initialized MIPI camera: " << device_path << std::endl;

    // 启动摄像头
    ret = mipi_camera.start();
    if (ret < 0) {
      std::cerr << "Failed to start MIPI camera" << std::endl;
      return;
    }


  }

  // 初始化VIPC服务器，使用实际分辨率
  VisionIpcServer vipc_server("camerad", nullptr, nullptr);
  vipc_server.create_buffers(VISION_STREAM_ROAD, VIPC_BUFFER_COUNT, actual_width, actual_height);

  // 初始化CameraBuf
  CameraBuf camera_buf;
  // 由于我们没有SpectraCamera实例，直接设置必要的成员变量
  camera_buf.vipc_server = &vipc_server;
  camera_buf.stream_type = VISION_STREAM_ROAD;
  camera_buf.out_img_width = actual_width;
  camera_buf.out_img_height = actual_height;
  camera_buf.cur_buf_idx = 0;

  // 使用局部变量跟踪缓冲区计数
  int frame_buf_count = VIPC_BUFFER_COUNT;

  // 初始化PubMaster
  PubMaster pm({"roadCameraState"});

  // 启动VIPC服务器监听
  vipc_server.start_listener();

  // 主循环
  uint32_t frame_id = 0;
  uint64_t last_frame_time = 0;
  uint64_t frame_interval = 1000000000ULL / framerate; // 帧间隔（纳秒）

  // 帧率统计
  static const int FRAME_STATS_WINDOW = 30; // 增加统计窗口以获得更准确的结果
  uint64_t frame_timestamps[FRAME_STATS_WINDOW] = {0};
  int frame_stats_idx = 0;

  while (!g_should_exit) {
    // 计算需要等待的时间以维持帧率
    uint64_t current_time = nanos_since_boot();
    uint64_t time_since_last_frame = current_time - last_frame_time;

    if (time_since_last_frame < frame_interval) {
      // 等待剩余时间（转换为微秒）
      usleep((frame_interval - time_since_last_frame) / 1000);
    }

    // 更新帧数据
    camera_buf.cur_frame_data.frame_id = frame_id++;
    last_frame_time = camera_buf.cur_frame_data.timestamp_sof = nanos_since_boot();

    // 获取当前的VIPC缓冲区
    camera_buf.cur_yuv_buf = camera_buf.vipc_server->get_buffer(camera_buf.stream_type, camera_buf.cur_buf_idx);

    // 读取帧数据
    if (use_webcam) {
      ret = usb_camera.read_frame(&camera_buf);
    } else {
      ret = mipi_camera.read_frame(&camera_buf);
    }
    if (ret < 0) {
      std::cerr << "Failed to read frame" << std::endl;
      continue;
    }

    // 发送帧到VIPC
    VisionIpcBufExtra extra = {
      camera_buf.cur_frame_data.frame_id,
      camera_buf.cur_frame_data.timestamp_sof,
      camera_buf.cur_frame_data.timestamp_eof,
    };
    camera_buf.cur_yuv_buf->set_frame_id(camera_buf.cur_frame_data.frame_id);
    camera_buf.vipc_server->send(camera_buf.cur_yuv_buf, &extra);

    // 更新时间戳
    camera_buf.cur_frame_data.timestamp_eof = nanos_since_boot();
    camera_buf.cur_frame_data.processing_time = camera_buf.cur_frame_data.timestamp_eof - camera_buf.cur_frame_data.timestamp_sof;

    // 发送消息
    MessageBuilder msg;
    auto framed = msg.initEvent().initRoadCameraState();
    framed.setFrameId(camera_buf.cur_frame_data.frame_id);
    framed.setTimestampSof(camera_buf.cur_frame_data.timestamp_sof);
    framed.setTimestampEof(camera_buf.cur_frame_data.timestamp_eof);
    framed.setProcessingTime(camera_buf.cur_frame_data.processing_time);

    // 发送消息
    pm.send("roadCameraState", msg);

    // 循环缓冲区索引
    camera_buf.cur_buf_idx = (camera_buf.cur_buf_idx + 1) % frame_buf_count;

    // 帧率统计
    frame_timestamps[frame_stats_idx] = nanos_since_boot();
    frame_stats_idx = (frame_stats_idx + 1) % FRAME_STATS_WINDOW;

    // 计算实际帧率
    if (debug_mode && frame_id > FRAME_STATS_WINDOW) {
      uint64_t oldest_time = frame_timestamps[0];
      uint64_t newest_time = frame_timestamps[0];
      for (int i = 0; i < FRAME_STATS_WINDOW; i++) {
        if (frame_timestamps[i] < oldest_time) oldest_time = frame_timestamps[i];
        if (frame_timestamps[i] > newest_time) newest_time = frame_timestamps[i];
      }

      if (newest_time > oldest_time) {
        float actual_fps = (FRAME_STATS_WINDOW * 1000000000.0f) / (newest_time - oldest_time);
        if (frame_id % FRAME_STATS_WINDOW == 0) {
          std::cout << "[DEBUG] Frame " << frame_id << ": "
                    << "Actual FPS: " << actual_fps << ", "
                    << "Expected FPS: " << framerate << ", "
                    << "Processing time: " << camera_buf.cur_frame_data.processing_time << "ms" << std::endl;
        }
      }
    }
  }

  // 清理资源
  if (use_webcam) {
    std::cout << "Stopping USB camera..." << std::endl;
    usb_camera.stop();
    std::cout << "USB camera stopped successfully" << std::endl;
  } else {
    std::cout << "Stopping MIPI camera..." << std::endl;
    mipi_camera.stop();
    std::cout << "MIPI camera stopped successfully" << std::endl;
  }
}
#endif
