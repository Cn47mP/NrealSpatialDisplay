# NrealSpatialDisplay

> Nreal Light 多虚拟屏空间锚定系统 — 将多个虚拟显示器固定在 3D 空间中，通过头部转动查看不同屏幕

[![Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-blue)]()
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![License](https://img.shields.io/badge/License-MIT-green)]()

支持 **World Lock / Head Lock** 两种模式，IMU 漂移校正，屏幕吸附对齐，曲面屏显示。中文 GUI，全热键可自定义。

---

## ✨ 特性

- **多虚拟屏空间锚定** — 最多 5 块虚拟屏幕锚定在 3D 空间中，转头即可查看不同方向
- **5 种预设布局** — 单屏 / 双屏 / 三屏环绕 / 五屏弧形 / 曲面超宽屏
- **双锁定模式** — World Lock（屏幕固定空间）+ Head Lock（屏幕跟随头部）
- **IMU 漂移校正** — 手动重置 + 自动低通滤波补偿
- **曲面屏支持** — 可调曲率的弧形屏幕网格生成
- **屏幕吸附对齐** — 移动屏幕时自动吸附到相邻屏幕边缘
- **每屏独立分辨率** — 每块虚拟屏可设置不同的分辨率
- **实时性能 HUD** — FPS、端到端延迟、IMU 状态、捕获状态叠加显示
- **中文设置面板** — 基于 Dear ImGui 的全中文 GUI，7 个标签页覆盖所有参数
- **热键冲突检测** — 可视化热键配置，实时冲突检测与提示
- **系统托盘** — 最小化到托盘，右键快速切换布局
- **截图** — 一键保存当前眼镜画面为 BMP

---

## 📐 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        Windows PC                                │
│                                                                  │
│  Virtual Display Driver ──→ N 个虚拟显示器                        │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐ │
│  │ WCG 桌面捕获  │  │ AirAPI IMU   │  │ D3D11 渲染引擎         │ │
│  │ (每屏一个)    │  │ 四元数 250Hz │  │ 离屏 RT → Blit        │ │
│  └──────┬───────┘  └──────┬───────┘  │ → 眼镜 SwapChain      │ │
│         │                  │          │ → 预览 SwapChain       │ │
│         └──────────┬───────┘          │ → HUD 叠加             │ │
│                    │                  └────────────────────────┘ │
│           ┌────────▼────────┐                                    │
│           │  Layout Engine  │  3D 空间布局 + 吸附对齐             │
│           │  Camera (IMU)   │  World Lock / Head Lock            │
│           └─────────────────┘                                    │
│                                                                  │
│  ┌─────────────┐  ┌──────────────┐                               │
│  │ 预览窗口     │  │ 系统托盘      │                               │
│  │ + HUD       │  │ + 设置面板    │                               │
│  └─────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                    USB-C (1920×1080, 52° FOV)
                              │
                    ┌─────────▼─────────┐
                    │   Nreal Light      │
                    └────────────────────┘
```

---

## 🛠 依赖

| 组件 | 来源 | 用途 |
|---|---|---|
| AirAPI_Windows.dll | [MSmithDev/AirAPI_Windows](https://github.com/MSmithDev/AirAPI_Windows/releases) | 读取 Nreal IMU 数据 |
| hidapi.dll | [libusb/hidapi](https://github.com/libusb/hidapi/releases) | USB HID 通信 |
| Virtual Display Driver | [19980202yyq/Virtual-Display-Driver](https://github.com/19980202yyq/Virtual-Display-Driver) | 创建虚拟显示器 |
| Dear ImGui | [ocornut/imgui](https://github.com/ocornut/imgui) | 设置面板 GUI（自动下载） |
| nlohmann/json | [nlohmann/json](https://github.com/nlohmann/json) | JSON 配置解析 |
| VirtualDesktopAccessor（可选） | [Ciantic/VirtualDesktopAccessor](https://github.com/Ciantic/VirtualDesktopAccessor) | 虚拟桌面切换 |

---

## 🚀 快速开始

### 前置条件

- Windows 10/11（1803+）
- Visual Studio 2022（C++17）
- NVIDIA 显卡（驱动 ≥ 530，推荐 Studio 驱动）
- Nreal Light 通过 USB-C 连接
- Virtual Display Driver 已安装

### 构建

```bash
# 1. 下载 ImGui 源码
cd src/gui && bash download_imgui.sh && cd ../..

# 2. 放置依赖到 lib/
#    - AirAPI_Windows.dll + .lib + .h
#    - hidapi.dll
#    - json.hpp (nlohmann)
#    - VirtualDesktopAccessor.dll（可选）

# 3. CMake 构建
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 运行

```bash
# 管理员权限运行
NrealSpatialDisplay.exe
```

1. 程序自动查找 Nreal Light 显示器，找不到则进入窗口开发模式
2. 系统托盘出现图标，右键可切换布局
3. 按 **F1** 打开设置面板调整参数
4. 按 **F3** 打开预览窗口查看眼镜画面
5. 将应用窗口拖到虚拟显示器上，眼镜中即可看到多屏空间布局
6. 转头查看不同方向的屏幕

---

## ⌨️ 快捷键

| 快捷键 | 功能 |
|---|---|
| `F1` | 显示/隐藏设置面板 |
| `F3` | 显示/隐藏预览窗口 |
| `F4` | 切换 Head Lock / World Lock |
| `F5` | 显示/隐藏 HUD |
| `Ctrl+Alt+1` | 单屏模式 |
| `Ctrl+Alt+2` | 双屏模式 |
| `Ctrl+Alt+3` | 三屏环绕 |
| `Ctrl+Alt+5` | 五屏弧形 |
| `Ctrl+Alt+I` | 切换 IMU |
| `Ctrl+Alt+R` | 重置视角 + 漂移校正 |
| `Ctrl+Alt+S` | 截图（BMP） |
| `Ctrl+Alt+Q` | 退出程序 |
| `Ctrl+Alt+↑/↓` | 亮度调节 |

所有热键可在 GUI 中自定义，支持冲突检测。

---

## 📁 项目结构

```
NrealSpatialDisplay/
├── CMakeLists.txt
├── config/
│   └── default.json                # 配置文件
├── lib/                            # 依赖库
├── src/
│   ├── main.cpp                    # WinMain 入口
│   ├── app/                        # 应用主体（托盘、热键、配置）
│   ├── gui/                        # ImGui 设置面板 + HUD
│   ├── imu/                        # IMU 读取 + 滤波 + 漂移校正
│   ├── capture/                    # WCG 桌面捕获
│   ├── render/                     # D3D11 渲染引擎 + 着色器
│   ├── layout/                     # 3D 布局引擎 + 预设
│   └── utils/                      # COM 辅助 + 日志
└── README.md
```

---

## 🎮 布局预设

| 预设 | 屏幕数 | 描述 | 观察距离 |
|---|---|---|---|
| single | 1 | 单屏正对前方 | 2.0m |
| dual | 2 | 左右双屏微展 20° | 1.8m |
| triple | 3 | 左中右三屏环绕（±30°） | 1.5~2.0m |
| five-arc | 5 | 五屏弧形（±50° / ±25° / 0°） | 1.0~2.0m |
| curved-ultrawide | 1 | 曲面超宽屏（1.6m 宽，曲率 0.4） | 1.8m |

---

## 🔧 配置

配置文件位于 `config/default.json`，包含以下模块：

```jsonc
{
  "version": 1,
  "imu": {
    "enabled": true,
    "smoothing": 0.15,           // 平滑系数 (0.10~0.20)
    "deadzone_deg": 0.5,         // 死区阈值 (0.3°~1.0°)
    "autoDriftCompensation": false
  },
  "displays": {
    "nrealName": "light",
    "virtualCount": 3,
    "virtualResolution": { "width": 1920, "height": 1080 }
  },
  "layout": {
    "preset": "triple",
    "viewerDistance": 2.0,
    "screens": [ /* 每屏的 position、rotation、size、curvature */ ]
  },
  "rendering": {
    "vsync": true,
    "showBorders": true,
    "sharpenEnabled": false,
    "sharpenStrength": 0.5
  },
  "hotkeys": [ /* 可自定义热键绑定 */ ]
}
```

所有配置均可通过 GUI 设置面板修改，无需手动编辑 JSON。

---

## 🐛 故障排查

| 问题 | 排查方向 |
|---|---|
| 眼镜黑屏 | 检查 USB 连接（VID_3318）、Windows 显示设置、日志中 SwapChain 错误 |
| 画面卡顿 | 查看 GPU 占用、尝试关闭 VSync、切换平面布局对比 |
| 头部转动无反应 | 检查 HID 设备、IMU 开关、HUD 中 P/Y/R 数值是否变化 |
| 屏幕位置错乱 | 检查配置文件 captureIndex、重置布局 |
| 热键无响应 | 检查冲突标记、关闭其他全局热键程序、管理员权限运行 |

日志文件位于 `logs/nreal_YYYY-MM-DD.log`，按天滚动，自动保留 7 天。快速排查：搜索 `ERROR` 和 `WARN` 条目。

---

## 🆚 竞品对比

| 功能 | GingerXR | VertoXR | **NrealSpatialDisplay** |
|---|---|---|---|
| 多虚拟屏空间锚定 | ✅ | ✅ | ✅ |
| 曲面屏 | ✅ | ✅ | ✅ |
| IMU 漂移校正 | ✅ | ✅ | ✅ 手动+自动 |
| Head Lock | ✅ | ✅ | ✅ |
| 中文 GUI | ❌ | ❌ | ✅ |
| 热键冲突检测 | ❌ | ❌ | ✅ |
| **开源** | ❌ | ❌ | ✅ |

---

## 📄 License

MIT
