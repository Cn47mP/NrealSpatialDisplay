#!/bin/bash
# download_imgui.sh
# 下载 Dear ImGui 并解压到 src/gui/imgui/
# 用法: cd src/gui && bash download_imgui.sh

set -e

IMGUI_VERSION="1.91.8"
IMGUI_URL="https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.zip"
TARGET_DIR="imgui"
TEMP_DIR=$(mktemp -d)

echo "=== 下载 Dear ImGui v${IMGUI_VERSION} ==="

# 下载
echo "正在下载..."
curl -sL "${IMGUI_URL}" -o "${TEMP_DIR}/imgui.zip"

# 解压
echo "正在解压..."
cd "${TEMP_DIR}"
unzip -q imgui.zip

# 移动到目标目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TARGET_PATH="${SCRIPT_DIR}/${TARGET_DIR}"

if [ -d "${TARGET_PATH}" ]; then
    echo "清理旧版本..."
    rm -rf "${TARGET_PATH}"
fi

mv "imgui-${IMGUI_VERSION}" "${TARGET_PATH}"

# 清理
rm -rf "${TEMP_DIR}"

echo ""
echo "=== Dear ImGui v${IMGUI_VERSION} 已安装到 ${TARGET_PATH} ==="
echo ""
echo "目录结构:"
echo "  ${TARGET_PATH}/"
echo "  ├── imgui.h"
echo "  ├── imgui.cpp"
echo "  ├── imgui_draw.cpp"
echo "  ├── imgui_tables.cpp"
echo "  ├── imgui_widgets.cpp"
echo "  ├── imgui_demo.cpp"
echo "  └── backends/"
echo "      ├── imgui_impl_win32.h/cpp"
echo "      └── imgui_impl_dx11.h/cpp"
echo ""
echo "现在可以构建项目了:"
echo "  mkdir build && cd build"
echo "  cmake .. -G \"Visual Studio 17 2022\" -A x64"
echo "  cmake --build . --config Release"
