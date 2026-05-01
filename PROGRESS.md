# NrealSpatialDisplay 构建进度

> 最后更新: 2026-04-29 15:41

## 项目概况

基于开发文档搭建的 Nreal Light 多虚拟屏空间锚定系统，包含端到端延迟测量和断连自动恢复功能。

## 文件清单与状态

### ✅ 已完成

| 文件 | 行数 | 说明 |
|---|---|---|
| `src/utils/TimeHelper.h` | ~15 | `GetTimestampUs()` 时间戳工具 |
| `src/utils/ComHelper.h` | ~40 | COM RAII + HRESULT 宏 |
| `src/utils/Log.h` | ~110 | 日志模块（控制台+文件+调试器，按天滚动，7天清理） |
| `src/imu/ImuFilter.h` | ~170 | 四元数平滑、死区、漂移校正、Head Lock（header-only） |
| `src/imu/AirIMU.h` | ~35 | AirAPI 封装头文件 |
| `src/imu/AirIMU.cpp` | ~150 | HID 动态加载、250Hz 轮询、断连检测+通知、`Reconnect()`、`SetPollRate()` |
| `src/render/Camera.h` | ~80 | IMU 驱动相机，World Lock / Head Lock（header-only） |
| `src/render/ScreenQuad.h` | ~25 | 平面/曲面四边形头文件 |
| `src/render/ScreenQuad.cpp` | ~100 | 平面 quad + 曲面弧线生成 |
| `src/render/D3DRenderer.h` | ~100 | D3D11 渲染器头文件 |
| `src/render/D3DRenderer.cpp` | ~500 | 设备创建、SwapChain、离屏 RT、Blit 管线、内联 HLSL、截图 BMP |
| `src/render/shaders/screen_vs.hlsl` | ~20 | 场景顶点着色器 |
| `src/render/shaders/screen_ps.hlsl` | ~25 | 场景像素着色器（纹理+边框） |
| `src/capture/DisplayCapture.h` | ~30 | WCG 单屏捕获头文件 |
| `src/capture/DisplayCapture.cpp` | ~130 | WCG 捕获框架（WinRT ABI，需根据实际环境完善） |
| `src/capture/CaptureManager.h` | ~35 | 多屏捕获管理头文件 |
| `src/capture/CaptureManager.cpp` | ~140 | 捕获管理、`ReinitCapture()`、`ReinitAll()`、`Pause()/Resume()` |
| `src/layout/LayoutEngine.h` | ~40 | 布局引擎头文件 |
| `src/layout/LayoutEngine.cpp` | ~260 | JSON 加载/保存、5种预设、世界矩阵、吸附对齐、曲面网格 |
| `src/layout/LayoutPreset.h` | ~30 | 预设注册表 |
| `src/app/AppConfig.h` | ~55 | 配置结构体（Imu/Display/Rendering/Hud/Logging/Hotkey） |
| `src/app/AppConfig.cpp` | ~220 | JSON 加载/保存、版本迁移、14个默认热键 |
| `src/gui/SettingsGUI.h` | ~85 | ImGui 设置面板头文件 |
| `src/gui/SettingsGUI.cpp` | ~400 | 7个标签页、HUD、热键冲突检测、延迟显示、连接状态显示 |
| `src/app/Application.h` | ~80 | 主应用头文件（连接状态机、延迟统计） |

### ❌ 未完成

| 文件 | 预估行数 | 说明 |
|---|---|---|
| `src/app/Application.cpp` | ~500 | **核心**：Init、Tick、托盘、热键注册/分发、断连恢复、窗口过程 |
| `src/main.cpp` | ~30 | WinMain 入口 |
| `config/default.json` | ~80 | 默认配置文件（含 imu/rendering/hud/hotkeys/layout） |
| `CMakeLists.txt` | ~80 | CMake 构建（VS2022 x64、HLSL 编译、ImGui 集成） |
| `README.md` | ~100 | 快速开始指南 |

## 待写文件的具体设计

### Application.cpp（最关键）

需要实现的方法（头文件已声明）：

```
bool Init():
    1. CoInitializeEx
    2. LayoutPreset::RegisterDefaults()
    3. AppConfig::Load(m_configPath)
    4. CreateMainWindow()           → 注册窗口类 + 创建主窗口
    5. D3DRenderer::Init()          → 离屏 RT + 眼镜 SwapChain
    6. D3DRenderer::InitScreenResources(N)
    7. 曲面屏 InitCurvedScreen()
    8. CreatePreviewWindow() + D3DRenderer::InitPreview()
    9. SettingsGUI::Init()
   10. CaptureManager::Init()
   11. AirIMU::Start() + SetDisconnectCallback()
   12. LayoutEngine::UpdateWorldMatrices()
   13. Camera::SetPosition(viewerOffset)
   14. RegisterConfigurableHotkeys()
   15. CreateTrayIcon()

void Tick():
    0. 性能统计（FPS/帧延迟，用 GetTimestampUs）
    1. 连接状态检查（m_connState != Connected → TryReconnect()）
    2. 捕获健康检查（!IsHealthy → ReinitCapture）
    3. 总开关检查（m_paused → Sleep(100) return）
    4. IMU → Camera::UpdateFromIMU
    5. CaptureManager::CaptureAll
    6. for each: UpdateTexture(i, tex)
    7. BeginFrame → DrawScreen × N → GUI → EndFrame
    8. RenderPreviewHud → PresentPreview
    9. 延迟计算（GetTimestampUs 打点，60帧滑动平均）
   10. GUI 数据更新（SetFps/SetLatencyStats/SetImuData 等）

TryReconnect():
    - 每 1 秒尝试一次
    - m_imu.Reconnect() + m_captureManager.ReinitAll()
    - 成功 → Connected，失败 → 保持 Disconnected

HandleAction(action):
    - preset_single/dual/triple/five → SwitchLayout()
    - toggle_imu → m_imu.Start()/Stop()
    - reset_view → m_camera.ResetYawZero() + SetRotation(0,0,0)
    - toggle_headlock → m_camera.ToggleMode()
    - screenshot → TakeScreenshot()
    - toggle_gui → m_settingsGui.ToggleVisible()
    - toggle_preview → ShowWindow(m_previewHwnd, ...)
    - toggle_hud → ...
    - brightness_up/down → m_imu.SetBrightness()
    - quit → PostQuitMessage()

托盘:
    - Shell_NotifyIcon 创建
    - 右键菜单：切换布局/设置/预览/截图/暂停/退出
    - 双击显示/隐藏设置面板
    - 布局变更时更新 tooltip

窗口过程:
    - WM_HOTKEY → 查 m_vkToAction → HandleAction
    - WM_TRAYICON → 右键菜单/双击
    - WM_CLOSE → 最小化到托盘（不退出）
    - WM_DESTROY → PostQuitMessage

RegisterConfigurableHotkeys():
    - 遍历 m_config.hotkeys
    - RegisterHotKey(m_mainHwnd, HOTKEY_BASE+i, mod, vk)
    - m_vkToAction[HOTKEY_BASE+i] = name
```

### main.cpp

```cpp
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    AllocConsole();
    Application app;
    if (!app.Init()) { MessageBox(...); return 1; }
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        app.HandleMessage(msg);
        app.Tick();
    }
    app.Shutdown();
}
```

### config/default.json

按 AppConfig 结构填写完整默认值，参考开发文档第 4.10 节。

### CMakeLists.txt

- C++17、x64
- find_package 或手动链接 d3d11/dxgi/d3dcompiler
- ImGui 源码编译（src/gui/imgui/ 目录）
- HLSL 着色器编译为 .cso（dxc.exe）
- lib/ 下的 DLL 不参与编译，运行时加载

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

## 注意事项

1. **DisplayCapture.cpp** 中的 WCG 实现是框架代码，WinRT ABI 调用需要根据实际编译环境调整（`winrt/` 头文件、`IGraphicsCaptureItemInterop` 等）
2. **AirAPI_Windows.dll** 函数签名基于文档推断，实际 API 可能有差异，需对照 [MSmithDev/AirAPI_Windows](https://github.com/MSmithDev/AirAPI_Windows) 确认
3. ImGui 需要手动下载源码放到 `src/gui/imgui/`，或运行 `download_imgui.sh`
4. `lib/` 目录需要手动放置 AirAPI_Windows.dll/.lib/.h、hidapi.dll、json.hpp、VirtualDesktopAccessor.dll
