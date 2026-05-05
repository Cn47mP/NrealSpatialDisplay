# NrealSpatialDisplay 开发路线图
> 最后更新: 2026-05-05 (补充修正)
> 当前版本: v0.1.0-alpha
> 状态: 可编译运行，核心渲染就绪，待硬件联调

---
## 📊 当前状态
✅ **编译状态：100%通过**  
✅ **核心模块：渲染链路完全可用**  
✅ **运行模式：支持窗口开发模式（无Nreal设备也可运行）**  
✅ **桌面捕获：GDI BitBlt 可用，WGC 高性能方案代码已写好（待启用）**  
⚠️ **依赖缺失：lib/ 中缺少 AirAPI/hidapi 二进制文件，IMU 以模拟模式运行**  
❌ **硬件适配：待Nreal设备联调**

---
## ✅ 已完成功能清单
### 核心渲染模块
- [x] D3D11渲染器实现，离屏渲染+双SwapChain输出
- [x] 场景着色器inline编译，无需外部dxc依赖
- [x] 3D空间变换系统，支持World Lock/Head Lock两种模式
- [x] IMU四元数滤波、漂移校正算法
- [x] 平面/曲面屏渲染，支持自定义曲率、网格细分
- [x] 虚拟屏边框渲染、纹理采样
- [x] 多屏布局系统，5种预设布局支持
- [x] 屏幕吸附对齐算法

### 系统框架
- [x] Win32主窗口+预览窗口实现
- [x] 系统托盘、热键注册、全局快捷键支持
- [x] ImGui中文设置面板，7个标签页
- [x] 热键冲突检测框架
- [x] 日志系统，按天滚动存储
- [x] JSON配置加载/保存
- [x] 无设备友好弹窗提示，支持`--no-popup`静默启动参数

### 桌面捕获
- [x] GDI BitBlt 桌面捕获，兼容全 Windows 版本
- [x] DXGI 显示器枚举，自动检测分辨率
- [x] 捕获管理器（CaptureManager），多屏独立捕获、暂停/恢复、健康检查
- [ ] WGC 高性能捕获（代码已写好，当前被注释，需启用并适配 WinRT ABI）

### 其他特性
- [x] 端到端延迟测量框架
- [x] 断连自动恢复状态机
- [x] 一键截图功能
- [x] 虚拟桌面切换接口
- [x] 无设备窗口开发模式（`--no-popup`）

---
## 🐛 待解决问题列表
### 🔴 P0（功能阻塞，最高优先级）
| 问题ID | 问题描述 | 影响范围 | 修复难度 | 状态 |
|---|---|---|---|---|
| P0-1 | WGC 高性能捕获未启用 | 单屏精确捕获性能 | 中 | 🔵 可选（GDI BitBlt 已实现且功能可用，WGC 代码已写好被注释） |
| P0-2 | 无Nreal设备时无法测试完整功能 | 硬件依赖，需物理设备验证 | 高（硬件依赖） | ❌ 未开始 |

### 🟡 P1（体验优化，高优先级）
| 问题ID | 功能描述 | 影响范围 | 开发难度 | 状态 |
|---|---|---|---|---|
| P1-1 | ✅ GUI设置面板核心功能对接 | 布局切换、IMU参数、渲染参数调整已生效；注意"重置默认"按钮仍为 stub | 低 | ✅ 已完成（部分） |
| P1-2 | 预览窗口HUD时序不对，延迟显示1帧 | 体验问题 | 极低 | ❌ 未开始 |
| P1-3 | 热键自定义编辑功能 | 设置面板支持修改热键绑定、实时冲突检测 | 体验优化 | 低 | ❌ 未开始 |
| P1-4 | 系统托盘右键菜单 | 托盘添加布局切换、暂停/继续、退出等快捷操作 | 体验优化 | 低 | ❌ 未开始 |
| P1-5 | 自定义布局保存功能 | 支持用户调整屏幕位置/曲率后保存为自定义预设 | 核心体验 | 中 | ❌ 未开始 |
| P1-6 | 自动亮度调节功能 | 对接AirAPI实现Nreal眼镜亮度调节 | 设备控制 | 低 | ❌ 未开始 |
| P1-7 | 多配置文件管理 | 支持多用户配置切换、导入/导出JSON | 体验优化 | 低 | ❌ 未开始 |
| P1-8 | 虚拟桌面切换功能 | 对接VirtualDesktopAccessor，热键切换虚拟桌面 | 可选功能 | 低 | ❌ 未开始 |
| P1-9 | IMU数据滤波参数调优 | 优化头部追踪流畅度，减少抖动 | 体验优化 | 中 | ❌ 未开始 |
| P1-10 | Application.cpp 重复初始化 bug | Init() 中 Log::Init/Config::Load/CreateMainWindow 被执行两次，第二次缺少 noPopup 参数 | 功能缺陷 | 低 | ✅ 已修复 |
| P1-11 | SettingsGUI "重置默认"按钮 | 点击无响应，需实现 AppConfig::CreateDefaults 加载并刷新 GUI | 体验缺陷 | 低 | ❌ 未开始 |
| P1-12 | DisplaySwitcher::SetBrightness | stub，返回 false，需对接 AirAPI 亮度接口 | 设备控制 | 低 | ❌ 未开始 |

### 🟢 P2（优化项，可选）
| 问题ID | 问题描述 | 影响范围 | 修复难度 |
|---|---|---|---|
| P2-1 | 渲染管线缺少视锥体裁剪 | 多屏场景性能 | 中 |
| P2-2 | 纹理采样缺少锐化算法 | 画面清晰度 | 低 |
| P2-3 | 缺少性能统计面板 | 调试功能 | 低 |
| P2-4 | 支持自定义曲面屏参数保存 | 体验优化 | 低 |
| P2-5 | 支持多语言切换 | 体验优化 | 中 |

---

## 📌 已知限制与开发说明

### 当前捕获方案
- **当前实际方案**：GDI `BitBlt` 全屏捕获，兼容所有 Windows 版本，性能可用但不如 WGC
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

---
## 🗺️ 开发路线图
### 版本v0.2.0目标（支持真实设备运行）
1. 放置 AirAPI/hidapi 二进制依赖，验证 IMU 真实数据链路
2. 硬件联调，验证完整链路（IMU→捕获→渲染→眼镜输出）
3. 调优IMU滤波参数，保证头部追踪流畅度
4. 修复 SettingsGUI "重置默认"按钮、DisplaySwitcher::SetBrightness 等 stub
5. （可选）启用 WGC 高性能捕获，替代 GDI BitBlt

### 版本v0.3.0目标（功能完整）
1. 实现虚拟桌面切换功能
2. 支持自定义布局保存/加载
3. 实现自动亮度调节
4. 完善性能优化与裁剪

### 版本v1.0.0目标（正式版）
1. 全功能测试与稳定性优化
2. 安装包制作
3. 完整用户文档

---
## 🚀 编译&运行指南
### 编译环境
- Windows 10 1903+
- Visual Studio 2022 / Build Tools 2022
- CMake 3.20+

### 编译步骤
```powershell
# 下载ImGui
cd src/gui
Invoke-WebRequest -Uri "https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip" -OutFile "imgui.zip"
Expand-Archive -Path "imgui.zip" -DestinationPath "."
Move-Item -Path "imgui-1.91.8" -Destination "imgui"
Remove-Item "imgui.zip"
cd ../..

# 构建
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 运行
```powershell
# 普通运行，无设备时弹窗提示
.\Release\Release\NrealSpatialDisplay.exe

# 静默启动，无弹窗直接进入开发模式
.\Release\Release\NrealSpatialDisplay.exe --no-popup
```

---
## 🏗️ 技术架构
### 核心数据流
```
┌──────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  IMU 读取│────►│  3D 相机计算 │────►│  桌面捕获   │────►│  3D 渲染    │
│ (250Hz)  │     │  四元数滤波  │     │ (60fps)     │     │  多屏合成   │
└──────────┘     └─────────────┘     └─────────────┘     └──────┬──────┘
                                                               │
                        ┌───────────────────────────────────────┘
                        ▼
                ┌─────────────┐         ┌─────────────┐
                │  眼镜输出   │         │  预览窗口    │
                │ (SwapChain) │         │ (HUD叠加)    │
                └─────────────┘         └─────────────┘
```

### 核心模块依赖关系
| 模块 | 依赖 | 核心功能 |
|---|---|---|
| Application | 所有子模块 | 主循环、热键、托盘、状态管理 |
| D3DRenderer | DirectX 11 | 渲染引擎、多屏绘制、SwapChain管理 |
| DisplayCapture | WinRT / GDI | 桌面捕获、纹理上传 |
| AirIMU | AirAPI / HIDAPI | IMU数据读取、断连检测 |
| LayoutEngine | nlohmann/json | 布局管理、屏幕对齐、预设加载 |
| SettingsGUI | Dear ImGui | 设置面板、HUD渲染 |

## 🔌 硬件&软件依赖清单
### 必选硬件
- Nreal Light 眼镜
- 支持USB-C视频输出的Windows电脑（NVIDIA显卡推荐，驱动≥530）

### 必选软件
| 组件 | 版本要求 | 下载地址 |
|---|---|---|
| Windows SDK | ≥10.0.19041.0 | 随VS2022安装 |
| Nreal AirAPI 驱动 | 最新版 | https://github.com/MSmithDev/AirAPI_Windows/releases |
| Virtual Display Driver | 最新版 | https://github.com/19980202yyq/Virtual-Display-Driver/releases |
| hidapi.dll | ≥0.14.0 | https://github.com/libusb/hidapi/releases |
| nlohmann/json | ≥3.11.0 | https://github.com/nlohmann/json/releases |

### 可选软件
- VirtualDesktopAccessor.dll：用于虚拟桌面切换（可选）：https://github.com/Ciantic/VirtualDesktopAccessor/releases

### 可选硬件
- 外置USB HUB（扩展多个虚拟屏供电）

---
## 🐛 故障排查指南
### 编译错误
| 问题 | 解决方法 |
|---|---|
| 找不到`windows.graphics.capture.interop.h` | 升级Windows SDK到10.0.19041.0及以上版本 |
| 链接错误`无法解析的外部符号 D3DCompile` | 确认CMakeLists.txt中已添加`d3dcompiler.lib`依赖 |
| ImGui头文件找不到 | 确认已运行下载脚本，`src/gui/imgui/`目录存在且包含imgui.h |

### 运行错误
| 问题 | 解决方法 |
|---|---|
| 启动后直接崩溃 | 以管理员权限运行，检查`logs/`目录下的日志文件定位错误 |
| 找不到Nreal设备 | 检查USB连接，确认AirAPI驱动已正确安装，设备管理器中能看到Nreal HID设备 |
| 眼镜有显示但画面不动 | 检查IMU权限，确认已授予HID设备访问权限 |
| 虚拟显示器不显示 | 确认Virtual Display Driver已正确安装，Windows显示设置中能看到虚拟显示器 |
| 画面撕裂 | 开启VSync，确认Nreal眼镜刷新率设置为60Hz |

### 调试技巧
1. 开启Debug模式编译，会启用D3D调试层，输出详细渲染错误
2. 查看`logs/nreal_YYYY-MM-DD.log`日志，搜索`ERROR`/`WARN`关键词
3. 无设备时使用`--no-popup`参数启动开发模式，调试渲染逻辑
4. 按F3开启预览窗口，查看渲染输出和HUD信息

---
## 🤝 贡献指南
### 代码规范
- 遵循现有代码风格，使用4空格缩进，UTF-8编码
- 函数名使用大驼峰，变量名使用小驼峰
- 避免添加冗余注释，代码自解释优先
- 提交前确认编译无警告，功能测试通过

### 提交规范
```
<类型>: <描述>

类型选项：
feat: 新功能
fix: 修复bug
docs: 文档更新
refactor: 重构
perf: 性能优化
chore: 构建/工具链修改
```

### 测试要求
- 窗口开发模式必须可正常运行
- 核心渲染逻辑修改必须验证3D变换生效
- 涉及Win32 API调用必须测试Windows 10/11兼容性
