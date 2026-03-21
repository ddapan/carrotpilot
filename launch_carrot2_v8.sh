#!/bin/bash
sudo chmod -R 777 /dev/bus/usb/*
#export BYD_RADAR=1
export FINGERPRINT="BYD_SONG_PLUS_DMI_21"
OP_DIR=$(dirname "$(readlink -f "$0")")
export PARAMS_ROOT="$OP_DIR/params"
export LOG_ROOT="$OP_DIR/.comma"
cd "$OP_DIR" &&
export CAMERAD_DEBUG=1
#export ROAD_CAM_WIDTH=1920
#export ROAD_CAM_HEIGHT=1080
export ROAD_CAM_FRAMERATE=22

source .venv/bin/activate &&
USE_WEBCAM=1 ROAD_CAM=0 system/manager/manager.py
#USE_WEBCAM=1 ROAD_CAM=2 selfdrive/debug/uiview.py