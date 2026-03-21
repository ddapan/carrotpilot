#!/bin/bash
# 从 vanjoge/carrotpilot 仓库同步 commits 的脚本

set -e

echo "正在从 vanjoge/carrotpilot 下载补丁文件..."

# 最新的 commit ID（从网页获取）
COMMIT_ID="61c6573"

# 创建临时目录
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo "临时目录: $TEMP_DIR"

# 尝试下载 patch 文件
# 方法1: 使用 curl 直接从 GitHub 下载（可能需要代理）
echo "尝试方法1: 直接下载..."
if curl -L -o vanjoge.patch "https://github.com/vanjoge/carrotpilot/compare/huheas:carrot2-v8...vanjoge:carrot2-v8.patch" --connect-timeout 10 --max-time 30 2>/dev/null; then
    echo "下载成功！"
    PATCH_FILE="$TEMP_DIR/vanjoge.patch"
else
    echo "方法1失败"

    # 方法2: 尝试使用 wget
    echo "尝试方法2: 使用 wget..."
    if wget -O vanjoge.patch "https://github.com/vanjoge/carrotpilot/compare/huheas:carrot2-v8...vanjoge:carrot2-v8.patch" --timeout=30 2>/dev/null; then
        echo "下载成功！"
        PATCH_FILE="$TEMP_DIR/vanjoge.patch"
    else
        echo ""
        echo "自动下载失败。请手动执行以下步骤："
        echo "1. 在浏览器中访问: https://github.com/vanjoge/carrotpilot/compare/huheas:carrot2-v8...vanjoge:carrot2-v8.patch"
        echo "2. 将页面内容保存为 vanjoge.patch 文件"
        echo "3. 将文件放到此目录: $TEMP_DIR"
        echo "4. 然后运行: cd /data/carrotpilot-v8 && git apply $TEMP_DIR/vanjoge.patch"
        echo ""
        echo "或者使用以下命令下载（如果你有代理）:"
        echo "curl -x YOUR_PROXY -L -o $TEMP_DIR/vanjoge.patch 'https://github.com/vanjoge/carrotpilot/compare/huheas:carrot2-v8...vanjoge:carrot2-v8.patch'"
        exit 1
    fi
fi

# 检查补丁文件
if [ -f "$PATCH_FILE" ] && [ -s "$PATCH_FILE" ]; then
    echo "补丁文件大小: $(wc -c < "$PATCH_FILE") 字节"
    echo ""
    echo "应用补丁到 /data/carrotpilot-v8 ..."

    cd /data/carrotpilot-v8

    # 先检查补丁是否可以应用
    if git apply --check "$PATCH_FILE" 2>/dev/null; then
        echo "补丁检查通过，开始应用..."
        git apply "$PATCH_FILE"
        echo ""
        echo "✓ 补丁已成功应用！"
        echo ""
        echo "vanjoge 仓库的更改已同步到本地"
    else
        echo "补丁无法直接应用，可能存在冲突。"
        echo "尝试使用 3-way merge..."
        git apply --3way "$PATCH_FILE" || {
            echo ""
            echo "应用失败。补丁文件保存在: $PATCH_FILE"
            echo "请手动检查并解决冲突。"
            exit 1
        }
    fi

    # 清理
    rm -rf "$TEMP_DIR"
    echo "已清理临时文件"
else
    echo "补丁文件下载失败或为空"
    exit 1
fi
