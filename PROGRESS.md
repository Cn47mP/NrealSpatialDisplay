# NrealSpatialDisplay 构建进度

> 最后更新: 2026-05-05

## 项目概况

基于开发文档搭建的 Nreal Light 多虚拟屏空间锚定系统，包含端到端延迟测量和断连自动恢复功能。完全由 AI 生成代码。

## 总体状态：可编译运行，核心渲染就绪，待硬件联调

所有核心模块已实现且可编译通过。GDI 桌面捕获可用，窗口开发模式（`--no-popup`）可正常运行。WGC 高性能捕获代码已写好但被注释，待启用。lib/ 中缺少 AirAPI/hidapi 二进制文件，IMU 以模拟模式运行。

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
| `src/capture/DisplayCapture.h` | 987 B | 桌面捕获头文件，**已完成** |
| `src/capture/DisplayCapture.cpp` | 3.3 KB | GDI BitBlt 桌面捕获已实现且可用；WGC 高性能捕获代码已写好但被注释（待启用），**已完成（GDI）** |
| `src/capture/CaptureManager.h` | 1.3 KB | 多屏捕获管理头文件，**已完成** |
| `src/capture/CaptureManager.cpp` | 4.7 KB | 捕获管理、ReinitCapture、Pause/Resume，**已完成** |
| `src/layout/LayoutEngine.h` | 1.5 KB | 布局引擎头文件，**已完成** |
| `src/layout/LayoutEngine.cpp` | 9.2 KB | JSON 加载/保存、5种预设、世界矩阵、吸附对齐、曲面网格，**已完成** |
| `src/layout/LayoutPreset.h` | 242 B | 预设注册表头文件，**已完成** |
| `src/layout/LayoutPreset.cpp` | 312 B | 预设注册，**已完成** |
| `src/display/DisplaySwitcher.h` | 1.4 KB | 显示切换器头文件，**已完成** |
| `src/display/DisplaySwitcher.cpp` | 5.3 KB | 虚拟桌面切换逻辑，大部分已完成（`SetBrightness` 为 stub），**大部分完成** |
| `src/network/UnityStream.h` | 907 B | Unity 数据流头文件，**已完成** |
| `src/network/UnityStream.cpp` | 3.5 KB | Unity 通信流，**已完成** |
| `src/gui/SettingsGUI.h` | 3.2 KB | ImGui 设置面板头文件，**已完成** |
| `src/gui/SettingsGUI.cpp` | 5.2 KB | 7个标签页、HUD、热键冲突检测、延迟显示（"重置默认"按钮为 stub），**已完成** |
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

### ✅ 已修复编译阻断问题

#### ✅ 问题 1：CMakeLists.txt 的 SOURCES 引用问题
已修复：移除不存在文件、添加缺失的 `NrealLightIMU.cpp`/`DisplaySwitcher.cpp`/`UnityStream.cpp`
已补充：添加 `d3dcompiler.lib` 链接依赖

#### ✅ 问题 2：Include 路径不一致
已修复：所有include路径已统一为相对src/的正确路径

#### ✅ 问题 4：CMakeLists.txt 缺少 HLSL 编译配置
已升级：改为inline HLSL编译方案，无需运行时依赖.cso文件，完全消除外部编译依赖

#### ✅ 问题 5：渲染链路不完整
已修复：
- `LoadSceneShaders()` 实现，inline编译场景着色器
- `DrawScreen()` 修复，使用真实场景着色器，worldViewProj矩阵3D变换生效
- `EndFrame()` 补全眼镜Blit+Present输出逻辑
- `InitCurvedScreen()` 参数不匹配bug修复
- 曲面屏/平面屏mesh绘制逻辑完整

---

## 已知限制与待解决问题

### 捕获方案说明
- **当前实际方案**：GDI `BitBlt` 全屏捕获，兼容所有 Windows 版本，性能可用
- **WGC 方案**：代码已写好（`DisplayCapture.cpp` 第 190-313 行），但被注释掉。启用需要：解除注释、适配 WinRT ABI 头文件、测试 `IGraphicsCaptureItemInterop` 激活
- **模拟帧**：当 GDI 捕获失败时自动回退到程序化波纹图案，仅用于调试

### lib/ 依赖说明
- `lib/` 目录当前只有 `json.hpp`，**缺少 AirAPI_Windows.dll/.lib 和 hidapi.dll**
- CMake 做了条件跳过（`if(EXISTS ...)`），缺少时编译仍可成功，但 IMU 模块会以**模拟模式**运行
- 正式使用需手动放置二进制依赖到 `lib/` 目录

### 无设备开发模式限制
使用 `--no-popup` 启动时的已知限制：
- IMU 无真实数据，以模拟模式运行（无头部追踪）
- 无眼镜 SwapChain 输出，只能通过预览窗口（F3）查看渲染结果
- GDI 捕获的是**主显示器**，而非虚拟显示器（虚拟显示器需 Virtual Display Driver）

### 待修复的 stub / bug
| 问题 | 位置 | 状态 |
|---|---|---|
| SettingsGUI "重置默认"按钮无响应 | `src/gui/SettingsGUI.cpp` 第 187 行 | ❌ 未开始 |
| DisplaySwitcher::SetBrightness 未实现 | `src/display/DisplaySwitcher.cpp` 第 161 行 | ❌ 未开始 |
| WGC 高性能捕获未启用 | `src/capture/DisplayCapture.cpp` 第 190-313 行 | 🔵 可选（GDI 已可用） |

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

1. **✅ P0（已完成）**：修复 CMakeLists.txt 的 SOURCES 列表
2. **✅ P0（已完成）**：统一 include 路径
3. **✅ P0（已完成）**：HLSL编译实现（inline方案）
4. **✅ P0（已完成）**：修复渲染链路（DrawScreen/EndFrame/InitCurvedScreen）
5. **✅ P0（已完成）**：GDI 桌面捕获实现
6. **✅ P0（已完成）**：Application.cpp 重复初始化 bug 修复
7. **✅ P1（已完成）**：无设备弹窗提示 + `--no-popup` 开发模式
8. **✅ P1（已完成）**：更新 PROGRESS.md / README.md
9. **🟡 P1**：SettingsGUI "重置默认"按钮实现
10. **🟡 P1**：DisplaySwitcher::SetBrightness 实现
11. **🔵 P1（可选）**：启用 WGC 高性能捕获
12. **🟢 P2**：渲染管线视锥体裁剪、锐化算法、性能面板等优化项
