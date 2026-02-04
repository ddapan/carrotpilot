# Camerad Thread 参数说明

## 概述

`camerad` 是一个高性能的摄像头采集服务，使用 C++ 实现，可以完全替代 `tools/webcam` 中使用 Python 的方式启动摄像头。

### 与 Python 版本对比

| 特性 | Python 版本 (tools/webcam) | C++ 版本 (camerad) |
|-----|--------------------------|-------------------|
| 性能 | 较慢 | **更优** |
| 语言 | Python | **C++** |
| MIPI 摄像头支持 | ❌ | ✅ |
| USB 摄像头支持 | ✅ | ✅ |
| 性能优化 | 基础 | **接近最优解** |

### 技术优势

1. **性能更优**: C++ 实现相比 Python 有显著的性能提升
2. **支持 MIPI 摄像头**: 除了 USB 摄像头，还支持 MIPI 摄像头
3. **性能最优解**:
   - **MIPI 摄像头**: 直接通过 V4L2 读取 NV12 格式，零拷贝，性能最优
   - **USB 摄像头**: 通过 libyuv 直接将 MJPEG 转换为 NV12。CPU指令集优化，无需GPU参与，GPU可更专注推理

### 使用前准备

**重要**: 使用 USB 摄像头前需要先自行编译 libyuv，相关脚本third_party/libyuv/build.sh已修改。

### 当前限制

⚠️ **注意**: 当前版本暂只支持 `ROAD_CAM`，其他摄像头类型暂未实现。

---

本文档说明 `camerad_thread.cc` 中使用的各个环境变量参数及其作用。

## 环境变量参数

### CAMERAD_DEBUG
- **类型**: 字符串
- **可选值**: `1`, `true`, `0`, `false`
- **默认值**: `false` (关闭)
- **作用**: 启用调试模式，输出详细的调试信息，包括：
  - 摄像头初始化信息
  - 设备路径选择过程
  - 分辨率和帧率设置
  - 实际帧率统计
  - 帧处理时间
- **示例**:
  ```bash
  export CAMERAD_DEBUG=1
  ```

### USE_WEBCAM
- **类型**: 字符串
- **可选值**: `1`, `true`, `0`, `false`
- **默认值**: `false` (使用MIPI摄像头)
- **作用**: 指定使用USB摄像头还是MIPI摄像头
  - `true`: 使用USB摄像头
  - `false`: 使用MIPI摄像头
- **示例**:
  ```bash
  export USE_WEBCAM=1
  ```

### ROAD_CAM_PATH
- **类型**: 字符串
- **默认值**: 无
- **优先级**: 最高
- **作用**: 直接指定摄像头设备的完整路径
- **说明**: 当设置此参数时，将忽略 `ROAD_CAM` 参数
- **示例**:
  ```bash
  export ROAD_CAM_PATH=/dev/video0
  ```

### ROAD_CAM
- **类型**: 字符串
- **默认值**: 无
- **优先级**: 中等 (低于 `ROAD_CAM_PATH`)
- **作用**: 指定摄像头设备编号，系统会自动拼接为 `/dev/video{ROAD_CAM}`
- **说明**: 当 `ROAD_CAM_PATH` 未设置时使用此参数
- **示例**:
  ```bash
  export ROAD_CAM=0  # 使用 /dev/video0
  ```

### ROAD_CAM_WIDTH
- **类型**: 字符串 (整数)
- **默认值**: `1920`
- **作用**: 设置摄像头采集的图像宽度
- **单位**: 像素
- **示例**:
  ```bash
  export ROAD_CAM_WIDTH=1920
  ```

### ROAD_CAM_HEIGHT
- **类型**: 字符串 (整数)
- **默认值**: `1080`
- **作用**: 设置摄像头采集的图像高度
- **单位**: 像素
- **示例**:
  ```bash
  export ROAD_CAM_HEIGHT=1080
  ```

### ROAD_CAM_FRAMERATE
- **类型**: 字符串 (整数)
- **默认值**: `20`
- **作用**: 设置摄像头采集的帧率
- **单位**: FPS (帧/秒)
- **说明**: 系统会根据此值计算帧间隔，并控制帧采集频率
- **示例**:
  ```bash
  export ROAD_CAM_FRAMERATE=30
  ```

## 默认设备路径

当未设置 `ROAD_CAM_PATH` 和 `ROAD_CAM` 时，系统根据 `USE_WEBCAM` 参数选择默认设备路径：

| USE_WEBCAM | 默认设备路径 | 摄像头类型 |
|-----------|-------------|-----------|
| `true` | `/dev/video45` | USB摄像头 |
| `false` | `/dev/video0` | MIPI摄像头 |

## 使用示例

### 示例1: 使用MIPI摄像头，默认配置
```bash
# 无需设置任何环境变量，使用默认配置
```

### 示例2: 使用USB摄像头，自定义分辨率
```bash
export USE_WEBCAM=1
export ROAD_CAM_WIDTH=1280
export ROAD_CAM_HEIGHT=720
export ROAD_CAM_FRAMERATE=30
```

### 示例3: 使用指定设备路径，开启调试模式
```bash
export ROAD_CAM_PATH=/dev/video2
export CAMERAD_DEBUG=1
export ROAD_CAM_FRAMERATE=25
```

### 示例4: 使用设备编号
```bash
export ROAD_CAM=1  # 使用 /dev/video1
export ROAD_CAM_WIDTH=1920
export ROAD_CAM_HEIGHT=1080
export ROAD_CAM_FRAMERATE=20
```

## 调试输出

当 `CAMERAD_DEBUG=1` 时，系统会输出以下信息：

1. **初始化信息**:
   - Debug模式启用状态
   - USB摄像头使用状态
   - 设备路径选择过程
   - 分辨率和帧率设置

2. **运行时信息**:
   - 实际帧率统计 (每30帧输出一次)
   - 帧处理时间
   - 帧ID和采集时间戳

3. **性能监控**:
   - Expected FPS: 期望帧率
   - Actual FPS: 实际帧率
   - Processing time: 帧处理时间 (纳秒)

## 注意事项

1. **设备路径优先级**: `ROAD_CAM_PATH` > `ROAD_CAM` > 默认路径
2. **分辨率自适应**: 系统会使用摄像头的实际分辨率，可能与设置值不同
3. **帧率控制**: 系统通过睡眠控制帧率，确保不超过设定值
4. **信号处理**: 支持 Ctrl+C 优雅退出
5. **缓冲区管理**: 使用循环缓冲区机制管理帧数据