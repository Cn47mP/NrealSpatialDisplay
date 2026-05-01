# NrealSpatialDisplay

Nreal Light 多虚拟屏空间锚定系统 —— 将多个虚拟显示器固定在 3D 空间中，通过头部转动查看不同屏幕。

## 功能特性

- **多虚拟屏**：支持 1/2/3/5 屏及曲面超宽屏布局
- **空间锚定**：屏幕固定在 3D 空间中，转头查看不同方向
- **IMU 驱动**：实时读取 Nreal Light IMU 数据（四元数）
- **DXGI 捕获**：低延迟桌面画面捕获
- **D3D11 渲染**：硬件加速多屏合成
- **布局引擎**：JSON 配置 + 预设布局 + 实时调整
- **设置面板**：Dear ImGui 中文 GUI，实时调整所有参数
- **预览窗口**：主显示器实时预览眼镜画面
- **性能 HUD**：预览窗口叠加 FPS/延迟/IMU 等信息
- **可配置热键**：所有热键均支持自定义修改
- **系统托盘**：最小化到托盘，右键菜单切换布局
- **截图**：离屏纹理直接保存为 BMP

## 硬件要求

- Windows PC
- Nreal Light (USB-C 连接)
- Virtual Display Driver

## 依赖

| 组件 | 来源 |
|---|---|
| AirAPI_Windows.dll | https://github.com/MSmithDev/AirAPI_Windows/releases |
| hidapi.dll | https://github.com/libusb/hidapi/releases |
| Virtual Display Driver | https://github.com/19980202yyq/Virtual-Display-Driver |
| nlohmann/json | https://github.com/nlohmann/json/releases |
| Dear ImGui | 自动下载 |

## 构建

```bash
# 1. 下载 ImGui
cd src/gui && bash download_imgui.sh && cd ../..

# 2. 放置依赖 DLL 到 lib/

# 3. 构建
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 4. 运行
NrealSpatialDisplay.exe
```

## 快捷键（默认值，可在 GUI 中自定义）

| 快捷键 | 功能 |
|---|---|
| F1 | 显示/隐藏设置面板 |
| F3 | 显示/隐藏预览窗口 |
| F4 | 切换 Head Lock / World Lock |
| F5 | 显示/隐藏 HUD |
| Ctrl+Alt+1 | 单屏模式 |
| Ctrl+Alt+2 | 双屏模式 |
| Ctrl+Alt+3 | 三屏模式 |
| Ctrl+Alt+5 | 五屏弧形 |
| Ctrl+Alt+I | 切换 IMU |
| Ctrl+Alt+R | 重置视角 + 漂移校正 |
| Ctrl+Alt+S | 截图 |
| Ctrl+Alt+Q | 退出 |

## 设置面板 (F1)

所有设置项均有中文说明，鼠标悬停 `(?)` 查看详细解释。

| 标签页 | 设置项 |
|---|---|
| 布局 | 预设选择（5种）、观察距离、重置/保存/退出 |
| 屏幕编辑 | 名称、捕获索引、位置XYZ、旋转PYR、尺寸宽高、曲率、快捷操作按钮 |
| IMU | 开关、平滑系数、死区、实时角度可视化条 |
| 渲染 | VSync、边框开关/宽度/颜色、背景色、固定参数显示 |
| 显示器 | 名称、虚拟屏数量、虚拟分辨率 |
| HUD | 开关、显示项目（FPS/IMU/捕获/布局）、热键绑定 |
| 快捷键 | 所有热键可视化配置、Ctrl/Alt/Shift 修饰键、**冲突检测**（红色高亮+提示） |

## 热键冲突检测

## 预览窗口 (F3)

- 主显示器置顶窗口，实时显示眼镜画面
- 可拖动移动
- 关闭不影响眼镜输出

## 性能 HUD (F5)

预览窗口左上角叠加显示：
- FPS / 帧延迟
- 布局名称 / 屏幕数量
- IMU 连接状态 / Pitch Yaw Roll
- 捕获源数量
- 渲染状态（运行中/已暂停）

## 热键冲突检测

快捷键标签页自动检测冲突：
- 冲突按键显示 **红色高亮** + `⚠` 标记
- 鼠标悬停查看冲突详情
- 顶部显示冲突数量统计
- 录制新键时自动拒绝已绑定的组合
- 修饰键（Ctrl/Alt/Shift）勾选时实时检测冲突
- 无冲突时显示 `✓ 无热键冲突`

## 核心特性

### 空间锚定 (World Lock)
屏幕固定在 3D 空间中，转头看到不同方向的屏幕。这是默认模式。

### Head Lock (F4)
屏幕跟随头部转动，像 HUD 一样始终在视野前方。适合单屏专注工作。
按 F4 切换模式，状态栏和 HUD 会显示当前模式。

### IMU 漂移校正
长时间使用后 IMU 可能产生缓慢的 yaw 漂移：
- **手动校正**：按 `Ctrl+Alt+R` 将当前朝向重置为零点
- **自动补偿**：在 IMU 设置中开启「自动漂移补偿」，系统自动检测并修正缓慢偏移

### 屏幕吸附对齐
在屏幕编辑器中移动屏幕时，会自动吸附到相邻屏幕的边缘（上下左右前后），辅助精确排列。

### 每屏独立分辨率
每个虚拟屏幕可以设置独立的捕获分辨率（在屏幕编辑器中），适合不同屏幕需要不同清晰度的场景。设为 0 则使用全局默认值。

## 渲染架构

```
场景渲染 → 离屏纹理 → ┬→ Blit → 眼镜 SwapChain → Present
                       └→ Blit → 预览 BackBuffer
                                      ↓
                                  HUD 叠加渲染
                                      ↓
                                  Present 预览
```

## 系统托盘

程序最小化到系统托盘，右键菜单提供：
- 切换布局（5 种预设，带当前选中标记）
- 打开设置面板 / 预览窗口
- 截图
- 暂停/启动渲染
- 退出

双击托盘图标显示/隐藏设置面板。

## 截图

按 `Ctrl+Alt+S` 或从托盘菜单触发，将当前眼镜画面保存为 BMP 文件（`screenshot_YYYYMMDD_HHMMSS.bmp`）。

## 项目结构

```
NrealSpatialDisplay/
├── CMakeLists.txt
├── config/default.json           # 含热键 + HUD 配置
├── lib/                          # 依赖 DLL
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── Application.cpp/h     # 主应用（可配置热键 + 预览 + HUD）
│   │   └── AppConfig.cpp/h       # 配置（含 HotkeyEntry + HudConfig）
│   ├── gui/
│   │   ├── SettingsGUI.cpp/h     # GUI（热键编辑 + HUD 设置 + 预览 HUD）
│   │   ├── ImGuiSetup.h
│   │   ├── download_imgui.sh
│   │   └── imgui/                # [自动下载]
│   ├── imu/
│   │   ├── AirIMU.cpp/h
│   │   └── ImuFilter.cpp/h
│   ├── capture/
│   │   ├── DisplayCapture.cpp/h
│   │   └── CaptureManager.cpp/h
│   ├── render/
│   │   ├── D3DRenderer.cpp/h     # 离屏 RT + 双 SwapChain + Blit
│   │   ├── ScreenQuad.cpp/h
│   │   ├── Camera.cpp/h
│   │   └── shaders/
│   ├── layout/
│   │   ├── LayoutEngine.cpp/h
│   │   └── LayoutPreset.cpp/h
│   └── utils/
│       ├── ComHelper.cpp/h
│       └── Log.cpp/h
└── README.md
```
