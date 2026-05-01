# NrealSpatialDisplay 构建进度

> 最后更新: 2026-05-02

## 项目概况

基于开发文档搭建的 Nreal Light 多虚拟屏空间锚定系统，包含端到端延迟测量和断连自动恢复功能。完全由 AI 生成代码。

## 总体状态：代码基本完成，但无法编译

所有核心模块的源文件已创建，配置文件和文档也已齐全。但存在**编译阻断问题**和**仓库膨胀问题**，需要修复后才能构建。

---

## 文件清单与状态

### ✅ 已完成（可编译的文件）

| 文件 | 大小 | 说明 |
|---|---|---|
| `src/main.cpp` | 1.2 KB | WinMain 入口，AllocConsole + 消息循环，**已完成** |
| `src/app/Application.h` | 3.0 KB | 主应用头文件，声明 Init/Tick/HandleAction/托盘/热键等，**已完成** |
| `src/app/Application.cpp` | 27 KB | 核心实现：Init（15步初始化）、Tick（10步帧循环）、TryReconnect、托盘菜单、热键注册/分发、窗口过程，**已完成** |
| `src/app/AppConfig.h` | 1.4 KB | 配置结构体定义（Imu/Display/Render/Hud/Hotkey），**已完成** |
| `src/app/AppConfig.cpp` | 4.0 KB | JSON 加载/保存/版本迁移/默认热键，**已完成** |
| `src/imu/AirIMU.h` | 1.8 KB | AirAPI HID 封装头文件，**已完成** |
| `src/imu/AirIMU.cpp` | 4.2 KB | HID 动态加载、250Hz 轮询、断连检测+通知、Reconnect()，**已完成** |
| `src/imu/ImuFilter.h` | 4.0 KB | 四元数平滑、死区、漂移校正、Head Lock（header-only），**已完成** |
| `src/imu/NrealLightIMU.h` | 1.3 KB | Nreal Light IMU 备选实现头文件，**已完成** |
| `src/imu/NrealLightIMU.cpp` | 14 KB | Nreal Light IMU 备选实现，**已完成** |
| `src/render/D3DRenderer.h` | 3.3 KB | D3D11 渲染器头文件，**已完成** |
| `src/render/D3DRenderer.cpp` | 14.5 KB | 设备创建、SwapChain、离屏 RT、Blit 管线、内联 HLSL、截图 BMP，**已完成** |
| `src/render/Camera.h` | 2.3 KB | IMU 驱动相机，World Lock / Head Lock（header-only），**已完成** |
| `src/render/ScreenQuad.h` | 685 B | 平面/曲面四边形头文件，**已完成** |
| `src/render/ScreenQuad.cpp` | 3.8 KB | 平面 quad + 曲面弧线生成，**已完成** |
| `src/render/shaders/screen_vs.hlsl` | 450 B | 场景顶点着色器，**已完成** |
| `src/render/shaders/screen_ps.hlsl` | 587 B | 场景像素着色器（纹理+边框），**已完成** |
| `src/capture/DisplayCapture.h` | 987 B | WCG 单屏捕获头文件，**已完成** |
| `src/capture/DisplayCapture.cpp` | 3.3 KB | WCG 框架代码，WinRT ABI 需适配（见下方问题），**部分完成** |
| `src/capture/CaptureManager.h` | 1.3 KB | 多屏捕获管理头文件，**已完成** |
| `src/capture/CaptureManager.cpp` | 4.7 KB | 捕获管理、ReinitCapture、Pause/Resume，**已完成** |
| `src/layout/LayoutEngine.h` | 1.5 KB | 布局引擎头文件，**已完成** |
| `src/layout/LayoutEngine.cpp` | 9.2 KB | JSON 加载/保存、5种预设、世界矩阵、吸附对齐、曲面网格，**已完成** |
| `src/layout/LayoutPreset.h` | 242 B | 预设注册表头文件，**已完成** |
| `src/layout/LayoutPreset.cpp` | 312 B | 预设注册，**已完成** |
| `src/display/DisplaySwitcher.h` | 1.4 KB | 显示切换器头文件，**已完成** |
| `src/display/DisplaySwitcher.cpp` | 5.3 KB | 虚拟桌面切换逻辑，**已完成** |
| `src/network/UnityStream.h` | 907 B | Unity 数据流头文件，**已完成** |
| `src/network/UnityStream.cpp` | 3.5 KB | Unity 通信流，**已完成** |
| `src/gui/SettingsGUI.h` | 3.2 KB | ImGui 设置面板头文件，**已完成** |
| `src/gui/SettingsGUI.cpp` | 5.2 KB | 7个标签页、HUD、热键冲突检测、延迟显示，**已完成** |
| `src/gui/ImGuiSetup.h` | 343 B | ImGui 初始化辅助头文件，**已完成** |
| `src/gui/download_imgui.sh` | 1.4 KB | ImGui 源码下载脚本，**已完成** |
| `src/utils/Log.h` | 4.3 KB | 日志模块（控制台+文件+调试器，按天滚动，7天清理），**已完成** |
| `src/utils/Log.cpp` | 217 B | 日志初始化（header-heavy），**已完成** |
| `src/utils/ComHelper.h` | 862 B | COM RAII + HRESULT 宏，**已完成** |
| `src/utils/ComHelper.cpp` | 67 B | stub（实际逻辑在 header），**已完成** |
| `src/utils/TimeHelper.h` | 467 B | GetTimestampUs() 时间戳工具，**已完成** |
| `config/default.json` | 5.4 KB | 完整默认配置（14个热键、三屏布局、IMU/渲染/HUD 参数），**已完成** |
| `CMakeLists.txt` | 2.0 KB | CMake 构建配置，**已完成但有问题（见下方）** |
| `README.md` | 9.8 KB | 完整文档（架构图、热键表、故障排查、竞品对比），**已完成** |

### ❌ 编译阻断问题

#### 问题 1：CMakeLists.txt 的 SOURCES 引用了不存在的文件

CMakeLists.txt 中列出了以下文件，但仓库中**不存在对应 .cpp**：

| SOURCES 中的路径 | 实际状态 | 修复方案 |
|---|---|---|
| `src/render/Camera.cpp` | **不存在**（header-only） | 从 SOURCES 移除，或创建空 stub |
| `src/imu/ImuFilter.cpp` | **不存在**（107B stub） | 从 SOURCES 移除 |
| `src/utils/ComHelper.cpp` | **存在但 67B stub** | 从 SOURCES 移除 |

同时 CMakeLists.txt **没有**引用这些已存在的文件：
- `src/imu/NrealLightIMU.cpp` ✅ 存在（14 KB）
- `src/display/DisplaySwitcher.cpp` ✅ 存在（5.3 KB）
- `src/network/UnityStream.cpp` ✅ 存在（3.5 KB）

#### 问题 2：Include 路径不一致

`Application.cpp` 和 `main.cpp` 使用 `#include "core/Log.h"` 格式，但实际文件在 `src/utils/Log.h`。CMakeLists.txt 中 include 路径设为 `${CMAKE_SOURCE_DIR}/src`，所以应该用 `utils/Log.h`、`app/AppConfig.h` 等路径。

涉及文件：
- `src/main.cpp`：`#include "core/Log.h"` → 应为 `"utils/Log.h"`
- `src/app/Application.cpp`：`#include "core/Log.h"` → `"utils/Log.h"`，`#include "core/AppConfig.h"` → `"app/AppConfig.h"`
- `src/app/Application.cpp`：`#include "stream/UnityStream.h"` → `"network/UnityStream.h"`

#### 问题 3：DisplayCapture.cpp 是框架代码

`src/capture/DisplayCapture.cpp`（3.3 KB）的 WCG 桌面捕获实现是**框架代码**，WinRT ABI 调用需要根据实际编译环境调整（`winrt/` 头文件、`IGraphicsCaptureItemInterop` 等）。这是项目能否实际运行的关键阻塞点。

#### 问题 4：CMakeLists.txt 缺少 HLSL 编译配置

README 提到使用 `dxc.exe` 编译 HLSL 着色器为 `.cso`，但 CMakeLists.txt 中没有相关编译规则。需要添加自定义命令将 `.hlsl` 编译为 `.cso` 并复制到输出目录。

---

## 仓库清理待办

| 问题 | 体积 | 说明 |
|---|---|---|
| `build/` 目录已提交 | 9.3 MB | CMake 构建产物，不应入仓库 |
| `src/gui/imgui/` 完整 vendor | 9.4 MB | 只需核心 + win32/dx11 backends（约 3 MB），不需要 examples/docs/misc |
| `airapi/hidapi-win/*.pdb` | 7.0 MB | 调试符号不应入仓库 |
| `airapi/AirAPI_Windows-master/` 源码 | ~200 KB | 未使用（项目链接预编译 DLL） |
| `.gitignore` 不完善 | — | 缺少 build/、*.obj、*.pdb、*.exe 等规则 |

运行 `cleanup_repo.sh`（见仓库根目录）可一键清理。

---

## 已集成的增强功能

### 端到端延迟测量
- `Application::Tick` 中用 `GetTimestampUs()` 在 IMU、捕获、渲染、Present 各环节打点
- 60 帧滑动平均，传给 `SettingsGUI::SetLatencyStats`
- HUD 显示：端到端 / 捕获 / 渲染 / Blit+Present 四项延迟

### 断连自动恢复
- `AirIMU`：`PollLoop` 检测 HID 读取失败 → 设置 `m_connected=false` → 触发 `m_disconnectCallback`
- `CaptureManager`：`CaptureAll` 中连续 60 帧失败 → 标记 `healthy=false`
- `Application`：`ConnectionState` 状态机，每 1 秒尝试 `Reconnect()` + `ReinitAll()`
- `SettingsGUI`：HUD 和状态栏显示红色 ⚠ 连接状态

---

## 修复优先级

1. **🔴 P0**：修复 CMakeLists.txt 的 SOURCES 列表（添加缺失文件、移除不存在的）
2. **🔴 P0**：统一 include 路径（`core/` → `utils/`/`app/`，`stream/` → `network/`）
3. **🟡 P1**：完成 DisplayCapture.cpp 的 WinRT 实现（需要实际 Windows 环境测试）
4. **🟡 P1**：添加 HLSL 编译规则到 CMakeLists.txt
5. **🟢 P2**：运行 cleanup_repo.sh 清理仓库
6. **🟢 P2**：更新 PROGRESS.md（本文件已完成）
