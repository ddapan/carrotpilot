#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

ARCHNAME=$(uname -m)
if [ -f /TICI ]; then
  ARCHNAME="larch64"
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
  ARCHNAME="Darwin"
fi

cd $DIR
if [ ! -d libyuv ]; then
  # 使用本地仓库而不是从远程 clone
  LOCAL_LIBYUV="/home/ddapan/下载/libyuv-refs_heads_main"
  if [ -d "$LOCAL_LIBYUV" ]; then
    echo "使用本地 libyuv 仓库: $LOCAL_LIBYUV"
    cp -r "$LOCAL_LIBYUV" libyuv
  else
    echo "错误: 本地 libyuv 仓库不存在: $LOCAL_LIBYUV"
    exit 1
  fi
fi

cd libyuv
# 本地仓库可能没有 git，跳过 checkout
# git checkout 917276084a49be726c90292ff0a6b0a3d571a6af

# build
cmake .
make -j$(nproc)

INSTALL_DIR="$DIR/$ARCHNAME"
rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

rm -rf $DIR/include
mkdir -p $INSTALL_DIR/lib
cp $DIR/libyuv/libyuv.a $INSTALL_DIR/lib
cp -r $DIR/libyuv/include $DIR

## To create universal binary on Darwin:
## ```
## lipo -create -output Darwin/libyuv.a path-to-x64/libyuv.a path-to-arm64/libyuv.a
## ```
