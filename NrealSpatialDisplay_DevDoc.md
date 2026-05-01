# NrealSpatialDisplay 完整开发文档

> Nreal Light 多虚拟屏空间锚定系统
> 将多个虚拟显示器固定在 3D 空间中，通过头部转动查看不同屏幕
> 支持 World Lock / Head Lock 两种模式，IMU 漂移校正，屏幕吸附对齐，曲面屏

---

## 1. 架构总览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Windows PC                                      │
│                                                                             │
│  ┌───────────────────┐                                                      │
│  │ Virtual Display    │  创建 N 个虚拟显示器                                  │
│  │ Driver             │  (Windows 显示设置中可见)                             │
│  └────────┬──────────┘                                                      │
│           │                                                                  │
│  ┌────────▼──────────┐  ┌───────────────┐  ┌─────────────────────┐         │
│  │ Windows.Graphics   │  │ AirAPI_Win    │  │ VirtualDesktop      │         │
│  │ .Capture (WCG)     │  │ dows.dll      │  │ Accessor.dll        │         │
│  │                    │  │               │  │                     │         │
│  │ 捕获每个虚拟       │  │ HID 读取 IMU  │  │ 切换虚拟桌面        │         │
│  │ 显示器画面         │  │ → 四元数      │  │ (备用方案)          │         │
│  └────────┬──────────┘  └───────┬───────┘  └─────────────────────┘         │
│           │                      │                                           │
│  ┌────────▼──────────────────────▼───────────────────────────────────────┐  │
│  │                          核心渲染引擎                                   │  │
│  │                                                                       │  │
│  │  ┌──────────────┐  ┌───────────────┐  ┌─────────────────────────┐    │  │
│  │  │ Layout       │  │ Camera        │  │ D3D11 Renderer          │    │  │
│  │  │ Engine       │  │               │  │                         │    │  │
│  │  │              │  │ World Lock /  │  │ 离屏渲染 → Blit         │    │  │
│  │  │ 5种预设      │  │ Head Lock     │  │ → 眼镜 SwapChain        │    │  │
│  │  │ 吸附对齐     │  │ 漂移校正      │  │ → 预览 SwapChain        │    │  │
│  │  │ 曲面支持     │  │ 死区+平滑     │  │ → HUD 叠加             │    │  │
│  │  └──────────────┘  └───────────────┘  └─────────────────────────┘    │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                        │                                     │
│  ┌─────────────────────────────────────┼─────────────────────────────────┐  │
│  │  ┌──────────────────┐              │                                 │  │
│  │  │ 预览窗口          │              ▼                                 │  │
│  │  │ (主显示器置顶)    │  ┌──────────────────────────────────────────┐  │  │
│  │  │ + HUD 性能叠加    │  │          Nreal Light 显示器              │  │  │
│  │  └──────────────────┘  │          (USB-C, 1920×1080, 52° FOV)     │  │  │
│  │                        │                                          │  │  │
│  │  ┌──────────────────┐  │  虚拟屏幕锚定在 3D 空间中                 │  │  │
│  │  │ 系统托盘          │  │  随头部转动而移动 (World Lock)           │  │  │
│  │  │ 设置面板 (GUI)    │  └──────────────────────────────────────────┘  │  │
│  │  └──────────────────┘                                                 │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 依赖组件

| 组件 | 来源 | 用途 |
|---|---|---|
| AirAPI_Windows.dll + .lib | [MSmithDev/AirAPI_Windows](https://github.com/MSmithDev/AirAPI_Windows/releases) | 读取 Nreal IMU 数据（四元数 + 欧拉角） |
| hidapi.dll | [libusb/hidapi](https://github.com/libusb/hidapi/releases) | USB HID 通信（AirAPI 依赖） |
| Fusion.lib | AirAPI_Windows 编译时自带 | IMU 传感器融合 |
| Windows.Graphics.Capture | Windows 10 1803+ 内置 API | 桌面捕获（替代 DXGI Desktop Duplication） |
| Virtual Display Driver | [19980202yyq/Virtual-Display-Driver](https://github.com/19980202yyq/Virtual-Display-Driver) | 创建虚拟显示器 |
| VirtualDesktopAccessor.dll（可选） | [Ciantic/VirtualDesktopAccessor](https://github.com/Ciantic/VirtualDesktopAccessor) | 虚拟桌面切换 |
| Dear ImGui（自动下载） | [ocornut/imgui](https://github.com/ocornut/imgui) | 设置面板 GUI |
| nlohmann/json | [nlohmann/json](https://github.com/nlohmann/json/releases) | 配置文件 JSON 解析 |

---

## 3. 项目结构

```
NrealSpatialDisplay/
├── CMakeLists.txt                          # CMake 构建（VS2022 x64）
├── config/
│   └── default.json                        # 完整配置文件
├── lib/                                    # 依赖 DLL + 静态库
│   ├── AirAPI_Windows.dll / .lib / .h
│   ├── hidapi.dll
│   ├── VirtualDesktopAccessor.dll
│   └── json.hpp
├── src/
│   ├── main.cpp                            # WinMain 入口
│   ├── app/
│   │   ├── Application.h                   # 主应用（托盘+预览+热键+截图）
│   │   ├── Application.cpp
│   │   ├── AppConfig.h                     # 配置结构体 + 加载/保存
│   │   └── AppConfig.cpp
│   ├── gui/
│   │   ├── SettingsGUI.h                   # ImGui 设置面板
│   │   ├── SettingsGUI.cpp                 # 全中文 + 热键冲突检测
│   │   ├── ImGuiSetup.h                    # ImGui 编译配置
│   │   ├── download_imgui.sh               # ImGui 一键下载
│   │   └── imgui/                          # [自动下载] Dear ImGui 源码
│   ├── imu/
│   │   ├── AirIMU.h                        # AirAPI_Windows 封装
│   │   ├── AirIMU.cpp
│   │   ├── ImuFilter.h                     # 平滑+死区+漂移校正+Head Lock
│   │   └── ImuFilter.cpp
│   ├── capture/
│   │   ├── DisplayCapture.h                # WCG 单屏捕获
│   │   ├── DisplayCapture.cpp
│   │   ├── CaptureManager.h                # 多屏捕获管理
│   │   └── CaptureManager.cpp
│   ├── render/
│   │   ├── D3DRenderer.h                   # D3D11 渲染引擎
│   │   ├── D3DRenderer.cpp                 # 离屏RT+双SwapChain+Blit+截图
│   │   ├── ScreenQuad.h                    # 平面/曲面四边形网格
│   │   ├── ScreenQuad.cpp
│   │   ├── Camera.h                        # IMU 驱动相机
│   │   ├── Camera.cpp
│   │   └── shaders/
│   │       ├── screen_vs.hlsl              # 顶点着色器
│   │       └── screen_ps.hlsl              # 像素着色器（纹理+边框）
│   ├── layout/
│   │   ├── LayoutEngine.h                  # 3D 布局引擎
│   │   ├── LayoutEngine.cpp
│   │   ├── LayoutPreset.h                  # 预设管理
│   │   └── LayoutPreset.cpp
│   └── utils/
│       ├── ComHelper.h                     # COM RAII + HRESULT 宏
│       ├── ComHelper.cpp
│       ├── Log.h                           # 日志工具
│       └── Log.cpp
└── README.md
```

---

## 4. 模块详解

### 4.1 IMU 封装 — AirIMU

**文件**：`src/imu/AirIMU.h/cpp`

通过 `AirAPI_Windows.dll` 读取 Nreal Light 的 IMU 数据。内部以 1000Hz 采样，降频到 250Hz 轮询。

```
AirAPI_Windows.dll
├── StartConnection()   → 建立 HID 连接
├── StopConnection()    → 断开连接
├── GetQuaternion()     → [x, y, z, w] 四元数
├── GetEuler()          → [pitch, roll, yaw] 欧拉角（度）
└── GetBrightness()     → 眼镜亮度
```

```cpp
struct ImuData {
    float quatX, quatY, quatZ, quatW;  // 四元数
    float pitch, roll, yaw;             // 欧拉角（度）
    uint64_t timestampUs;               // 时间戳（微秒）
};

class AirIMU {
    bool Start();                       // 连接 + 启动轮询线程
    void Stop();                        // 停止 + 断开
    bool IsConnected() const;
    ImuData GetLatest() const;          // 线程安全获取最新数据
    void SetCallback(ImuCallback cb);   // 异步回调（250Hz）
    int GetBrightness();

private:
    std::atomic<bool> m_running{false};
    std::thread m_pollThread;
    mutable std::mutex m_mutex;
    ImuData m_latest{};
    ImuCallback m_callback;
    void PollLoop();                    // 250Hz 轮询（Sleep(4)）
};
```

---

### 4.2 IMU 滤波 — ImuFilter

**文件**：`src/imu/ImuFilter.h`（header-only）

提供四元数平滑、死区滤波、yaw 漂移校正、Head Lock 支持。

#### 四元数平滑

使用线性插值（LERP）+ 归一化近似 SLERP，系数由 `smoothing` 控制（默认 0.15）。

#### 死区滤波

小幅抖动低于 `deadzone`（默认 0.5°）时忽略，减少静止时的画面漂移。

#### 漂移校正

IMU 的 yaw 角会随时间缓慢漂移（无外部参考系）。校正方式：

- **手动校正**：`ResetZeroReference(currentYawDeg)` 记录当前 yaw 作为零点偏移
- **自动补偿**：`EnableAutoDriftCompensation(true)` 启用低通滤波检测缓慢偏移，默认漂移率 0.001
- **应用校正**：每次 `UpdateQuaternion` 时，通过 `ApplyYawCorrection` 将偏移量叠加到 yaw 角

```
原始四元数 → 提取 yaw → 加上偏移 → 重构四元数 → 返回校正后结果
```

#### Head Lock 支持

```cpp
void LockCurrentOrientation(XMVECTOR currentQuat);  // 锁定当前朝向为锚点
void UnlockOrientation();                            // 解锁（回到 World Lock）
XMVECTOR GetHeadLockDelta(XMVECTOR currentQuat);    // 获取从锚点到当前的增量旋转
```

Head Lock 模式下，`GetHeadLockDelta` 返回 `inverse(locked) × current`，即从锁定点开始的相对旋转。

---

### 4.3 相机 — Camera

**文件**：`src/render/Camera.h`（header-only）

IMU 驱动的空间锚定相机，支持两种模式。

#### CameraMode

| 模式 | 行为 | 用途 |
|---|---|---|
| World Lock | 屏幕锚定空间，转头看到不同屏幕 | 多屏办公（默认） |
| Head Lock | 屏幕跟随头部转动，始终在视野前方 | 单屏 HUD / 固定视角 |

#### 核心接口

```cpp
class Camera {
    void UpdateFromIMU(const ImuData& imu);       // IMU → 更新旋转
    void SetRotation(float pitch, float yaw, float roll);  // 手动设置
    XMMATRIX GetViewMatrix() const;               // 获取 View 矩阵

    void SetMode(CameraMode mode);                // 切换模式
    void ToggleMode();                            // F4 热键切换
    CameraMode GetMode() const;

    void ResetYawZero();                          // 漂移校正（Ctrl+Alt+R）
    void EnableAutoDriftComp(bool enable);        // 自动漂移补偿
    void SetPosition(XMVECTOR pos);               // 设置位置（viewerOffset）
};
```

#### UpdateFromIMU 流程

```
IMU 四元数
    ↓
ImuFilter::UpdateQuaternion（平滑+死区+漂移校正）
    ↓
if WorldLock:
    m_rotation = filtered
if HeadLock:
    m_rotation = GetHeadLockDelta(filtered)  // 增量旋转
    ↓
GetViewMatrix() → LookAtLH(position, position+forward, up)
```

---

### 4.4 桌面捕获

**文件**：`src/capture/DisplayCapture.h/cpp`、`CaptureManager.h/cpp`

基于 Windows.Graphics.Capture (WCG) API，每个虚拟显示器对应一个 `DisplayCapture` 实例。相比 DXGI Desktop Duplication，WCG 支持捕获任意窗口/显示器，无需管理员权限，且兼容性更好。

```cpp
class DisplayCapture {
    bool Init(ID3D11Device* device, int displayIndex);  // 绑定显示器
    bool CaptureFrame(ID3D11DeviceContext* ctx,          // 捕获一帧
                      ID3D11Texture2D** outTexture);
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
};

class CaptureManager {
    bool Init(ID3D11Device* device, const std::vector<int>& displayIndices);
    void CaptureAll(ID3D11DeviceContext* ctx);           // 捕获所有显示器
    ID3D11Texture2D* GetTexture(size_t index) const;    // 获取最新帧
    size_t GetCount() const;
};
```

**WCG 捕获流程**：
```
CreateForMonitorId (每台虚拟显示器)
    ↓
创建 Direct3D11CaptureFramePool (GPU 端)
    ↓
每帧: TryGetNextFrame → GetSurface() → ID3D11Texture2D
    ↓
输出: ID3D11Texture2D（GPU 端纹理，可直接用于渲染）
```

---

### 4.5 D3D11 渲染器

**文件**：`src/render/D3DRenderer.h/cpp`、`ScreenQuad.h/cpp`、`shaders/*.hlsl`

核心渲染引擎，负责将多个虚拟屏幕合成到 Nreal Light 显示器。

#### 初始化

```cpp
bool Init(HWND glassesHwnd, UINT width, UINT height);   // 眼镜输出
bool InitPreview(HWND previewHwnd, UINT w, UINT h);     // 预览窗口
bool InitScreenResources(int screenCount);               // 创建 N 个纹理+SRV
bool InitCurvedScreen(int index, float w, float h,       // 创建曲面 quad
                       float curvature, int segments);

void BeginFrame();      // 清空离屏 RT，绑定场景着色器
void EndFrame();        // 离屏 → 眼镜 Present + 预览 Blit
void PresentPreview();  // HUD 渲染后 Present 预览

void DrawScreen(const XMMATRIX& worldMatrix, const XMMATRIX& viewProj,
                ID3D11ShaderResourceView* textureSRV, int screenIndex = -1);

void UpdateTexture(size_t index, ID3D11Texture2D* sourceTex);
ID3D11ShaderResourceView* GetTextureSRV(size_t index) const;

bool SaveScreenshot(const std::string& path);  // 截图 → BMP

void SetBackgroundColor(float r, float g, float b);
void SetBorderColor(float r, float g, float b);
void SetBorderWidth(float w);
void SetVSync(bool enabled);
bool IsPreviewReady() const;
```

#### 私有成员

```cpp
private:
    // D3D 核心（共享设备）
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;

    // 眼镜输出
    ComPtr<IDXGISwapChain1> m_glassesSwapChain;
    ComPtr<ID3D11RenderTargetView> m_glassesRTV;

    // 预览输出
    ComPtr<IDXGISwapChain1> m_previewSwapChain;
    ComPtr<ID3D11RenderTargetView> m_previewRTV;
    bool m_previewReady = false;

    // 离屏渲染目标
    ComPtr<ID3D11Texture2D> m_offscreenTex;
    ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
    ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;
    ComPtr<ID3D11DepthStencilView> m_dsv;

    // Blit 资源（内联 HLSL 编译）
    ComPtr<ID3D11VertexShader> m_blitVS;
    ComPtr<ID3D11PixelShader> m_blitPS;
    ComPtr<ID3D11Buffer> m_blitVB;

    // 场景着色器（从 .cso 加载）
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11Buffer> m_constantBuffer;

    // 屏幕纹理（每屏一个）
    std::vector<ComPtr<ID3D11Texture2D>> m_textures;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_srvs;

    // 共用 ScreenQuad + 每屏曲面 quad
    std::unique_ptr<ScreenQuad> m_sharedQuad;
    std::vector<std::unique_ptr<ScreenQuad>> m_screenQuads;
```

#### 渲染管线

```
1. BeginFrame()
   ├── 清空离屏 RT（背景色）
   ├── 绑定离屏 RT + 深度缓冲
   └── 设置场景视口 + 着色器

2. DrawScreen(worldMatrix, viewProj, textureSRV, screenIndex) × N
   ├── 更新常量缓冲（WorldViewProj + 边框参数）
   ├── 恢复场景着色器 + 离屏 RT（可能被 Blit 覆盖）
   ├── 绑定屏幕纹理 SRV
   ├── 选择 quad：
   │   ├── screenIndex 有曲面 quad → m_screenQuads[i]
   │   └── 否则 → m_sharedQuad（平面）
   └── DrawIndexed

3. GUI 叠加（ImGui 渲染到离屏 RT）

4. EndFrame()
   ├── Blit 离屏纹理 → 眼镜 back buffer → Present
   └── Blit 离屏纹理 → 预览 back buffer（不 Present）

5. 预览 HUD（独立 ImGui 上下文）
   ├── 渲染到预览 back buffer
   └── PresentPreview()
```

#### 离屏渲染架构

场景不直接渲染到眼镜 swap chain，而是先渲染到离屏纹理（offscreen texture），再通过 Blit 分发到两个输出。好处：
- 场景只渲染一次，两个输出共享
- GUI 和 HUD 可以分别叠加到不同输出
- 截图直接从离屏纹理读取

#### 曲面 ScreenQuad

```cpp
class ScreenQuad {
    bool Init(ID3D11Device* device, float width, float height);        // 平面
    bool InitCurved(ID3D11Device* device, float width, float height,   // 曲面
                     float curvature, int segments);
    void Bind(ID3D11DeviceContext* ctx);
    UINT GetIndexCount() const;
};
```

**曲面数学**：
```
R = 1 / curvature                              // 弧半径
halfAngle = 0.5 × width × curvature           // 半弧角（弧度）
for each segment i:
    angle = (i/segments - 0.5) × width × curvature
    x = R × sin(angle)                        // 水平位置
    z = R × (cos(angle) - cos(halfAngle))     // 深度，中心 z=0
```

**曲面实现**：有曲率的屏幕使用独立的 `ScreenQuad::InitCurved`，基于弧线公式 `R = 1/curvature` 生成顶点。无曲率的屏幕共用平面 quad。

#### Blit 资源

Blit 着色器在运行时从内联 HLSL 编译（`D3DCompile`），不需要外部 .cso 文件：
```hlsl
// 顶点：直接传递位置和 UV
VS_OUT VSBlit(float3 p : POSITION, float2 u : TEXCOORD0) { ... }
// 像素：采样离屏纹理
float4 PSBlit(VS_OUT i) : SV_TARGET { return tex.Sample(sam, i.uv); }
```

#### 截图

```cpp
bool SaveScreenshot(const std::string& path);  // 离屏纹理 → staging → BMP
```

流程：创建 staging 纹理（CPU 可读）→ CopyResource → Map → 写入 BMP 文件头 + 像素数据 → Unmap。

#### 场景着色器

**screen_vs.hlsl**（顶点着色器）：
```hlsl
cbuffer CB : register(b0) {
    float4x4 g_worldViewProj;
    float4   g_borderColor;
    float    g_borderWidth;
};
VS_OUT VSMain(VS_IN input) {
    output.pos = mul(float4(input.pos, 1.0f), g_worldViewProj);
    output.uv  = input.uv;
}
```

**screen_ps.hlsl**（像素着色器）：
```hlsl
float4 PSMain(PS_IN input) : SV_TARGET {
    // 边框检测
    if (uv.x < border || uv.x > 1-border || uv.y < border || uv.y > 1-border)
        return g_borderColor;
    // 采样桌面纹理
    return g_texture.Sample(g_sampler, uv);
}
```

---

### 4.6 3D 布局引擎

**文件**：`src/layout/LayoutEngine.h/cpp`、`LayoutPreset.h/cpp`

管理多个虚拟屏幕在 3D 空间中的排列。

#### ScreenConfig

```cpp
struct ScreenConfig {
    int captureIndex;         // 虚拟显示器编号 (0-based)
    std::string name;         // 显示名称
    XMFLOAT3 position;        // 3D 位置（米）：X=左右 Y=上下 Z=前后
    XMFLOAT3 rotationDeg;     // 旋转（度）：X=俯仰 Y=偏航 Z=翻滚
    XMFLOAT2 sizeMeters;      // 物理尺寸（米）：宽 × 高
    float curvatureRad;       // 曲率（0=平面，0.4=明显弯曲）
    int curveSegments;        // 曲面分段数（>1 时使用曲面 quad）
    int customWidth;          // 每屏独立分辨率（0=使用全局默认）
    int customHeight;
    XMMATRIX worldMatrix;     // 运行时：scale × rotZ × rotX × rotY × translate
};
```

#### LayoutConfig

```cpp
struct LayoutConfig {
    std::string name;                    // 预设名
    std::vector<ScreenConfig> screens;   // 屏幕列表
    float viewerDistance;                 // 观察者距离
    XMFLOAT3 viewerOffset;               // 观察者偏移（应用到 Camera 位置）
};
```

#### 预设布局

| 预设 | 屏幕数 | 描述 | 观察距离 |
|---|---|---|---|
| single | 1 | 单屏正对前方 | 2.0m |
| dual | 2 | 左右双屏微展 20° | 1.8m |
| triple | 3 | 左中右三屏环绕（左 30° / 右 -30°） | 1.5~2.0m |
| five-arc | 5 | 五屏弧形（±50° / ±25° / 0°） | 1.0~2.0m |
| curved-ultrawide | 1 | 单块曲面超宽屏（1.6m 宽，曲率 0.4） | 1.8m |

#### 核心接口

```cpp
class LayoutEngine {
    // 配置
    static LayoutConfig LoadFromJson(const std::string& path);
    static void SaveToJson(const std::string& path, const LayoutConfig& config);
    static LayoutConfig CreatePreset(const std::string& name,
                                      const std::vector<int>& captureIndices);

    // 矩阵计算
    static void UpdateWorldMatrices(LayoutConfig& layout);

    // 屏幕调整
    static void MoveScreen(LayoutConfig& layout, int index, XMFLOAT3 delta);
    static void RotateScreen(LayoutConfig& layout, int index, XMFLOAT3 deltaDeg);
    static void ScaleScreen(LayoutConfig& layout, int index, float factor);

    // 吸附对齐
    static void SnapScreen(LayoutConfig& layout, int index, float threshold = 0.05f);

    // 曲面网格
    static std::vector<XMFLOAT3> GenerateCurvedMesh(
        float width, float height, float curvature, int segments);
};
```

#### 吸附对齐

`SnapScreen` 检测目标屏幕 6 个方向的边缘与其他屏幕的距离：
- **X 轴**：左→右、右→左、左→左、右→右
- **Y 轴**：上→下、下→上、上→上、下→下
- **Z 轴**：前后对齐

距离小于阈值（默认 5cm）时自动对齐。移动操作后自动调用。

---

### 4.7 设置面板 GUI

**文件**：`src/gui/SettingsGUI.h/cpp`、`ImGuiSetup.h`

基于 Dear ImGui 的中文设置面板，按 F1 显示/隐藏。

#### 初始化

```cpp
bool Init(void* hwnd, void* device, void* deviceContext);  // 主 GUI 上下文
bool InitPreviewHud(void* dev, void* ctx, void* hwnd);     // 预览 HUD 上下文
```

使用独立的 ImGui 上下文渲染 HUD，避免与主 GUI 冲突。自动加载系统中文字体（微软雅黑 → 宋体 → 黑体回退）。

#### 接口

```cpp
class SettingsGUI {
    bool Init(void* hwnd, void* device, void* deviceContext);
    void NewFrame();              // 绘制 GUI
    void Render();                // 提交到主上下文
    bool InitPreviewHud(void* dev, void* ctx, void* hwnd);  // 预览 HUD
    void RenderPreviewHud();      // 渲染到预览 swap chain

    // 状态数据
    void SetConfig(const AppConfig& cfg);
    void SetEnabled(bool v);      // 总开关
    void SetImuConnected(bool c);
    void SetImuData(float p, float y, float r);
    void SetFps(float f);
    void SetFrameLatency(float ms);
    void SetCaptureCount(int n);
    void SetScreenCount(int n);
    void SetLayoutName(const std::string& n);
    void SetCameraMode(const std::string& mode);

    // 回调
    void OnToggle(ToggleCallback);
    void OnConfigChanged(ConfigChangedCallback);
    void OnLayoutSwitch(LayoutSwitchCallback);
    void OnResetView(ResetViewCallback);
    void OnQuit(QuitCallback);
    void OnAction(ActionCallback);    // 通用动作回调

    void ToggleVisible();
};
```

#### 标签页

| 标签 | 设置项 |
|---|---|
| 布局 | 5 种预设选择（附描述）、观察距离、观察偏移 XYZ、重置/保存/退出 |
| 屏幕编辑 | 屏幕列表、名称、捕获索引、位置/旋转/尺寸/曲率、每屏独立分辨率、快捷操作（居中/移动/缩放/旋转，含吸附） |
| IMU | 开关、平滑系数（推荐 0.10~0.20）、死区（推荐 0.3°~1.0°）、自动漂移补偿、相机模式切换（World Lock / Head Lock）、实时 Pitch/Yaw/Roll 可视化条、亮度控制 |
| 渲染 | VSync、边框开关/宽度/颜色、背景色、固定参数显示（FOV/分辨率/刷新率）、文本锐化开关/强度 |
| 显示器 | 名称、虚拟屏数量、虚拟分辨率、状态监控（⚠ 异常提示） |
| HUD | 开关、显示项目（FPS/延迟/IMU/捕获/布局）、热键绑定 |
| 快捷键 | 全部热键可视化配置、Ctrl/Alt/Shift 修饰键勾选、冲突检测 |

#### 热键冲突检测

- 修饰键勾选时**实时检测**冲突，冲突则自动回滚
- 录制新键时**拒绝已绑定组合**，弹出冲突提示
- 冲突按钮**红色高亮** + ⚠ 前缀
- 鼠标悬停显示与谁冲突
- 顶部统计冲突数量，无冲突时显示 `✓ 无热键冲突`

#### 预览 HUD

独立 ImGui 上下文，渲染到预览 swap chain back buffer。显示内容：
```
PREVIEW HUD
─────────────────────
FPS: 60.0    16.67ms
端到端: 18.3ms  (IMU→画面)
布局: triple  [3屏]  World Lock
IMU: 已连接  亮度: 65%
  P:-2.3  Y:15.1  R:0.5
捕获: 3 个源
─────────────────────
● 渲染中
```

#### 总开关

设置面板底部居中大按钮：
- 运行中 → 红色 `■ 暂停渲染`
- 已暂停 → 绿色 `▶ 启动渲染`
- 暂停时跳过渲染循环，CPU 空转降至接近 0

#### 回调机制

```cpp
void OnToggle(ToggleCallback);           // 总开关
void OnConfigChanged(ConfigChangedCallback);  // 配置变更 → 保存+应用
void OnLayoutSwitch(LayoutSwitchCallback);    // 布局切换
void OnResetView(ResetViewCallback);          // 重置视角
void OnQuit(QuitCallback);                    // 退出
void OnAction(ActionCallback);                // 通用动作（如 Head Lock 切换）
```

---

### 4.8 系统托盘

**文件**：`src/app/Application.h/cpp`

程序最小化到系统托盘（`Shell_NotifyIcon`）。

#### 托盘菜单

```
├── 切换布局
│   ├── 单屏 (&1)        ✓（当前布局标记）
│   ├── 双屏 (&2)
│   ├── 三屏环绕 (&3)
│   ├── 五屏弧形 (&5)
│   └── 曲面超宽屏 (&C)
├── ─────────────
├── 设置面板  F1 (&G)
├── 预览窗口  F3 (&P)
├── 截图      Ctrl+Alt+S (&T)
├── ─────────────
├── 暂停渲染 (&U) / 启动渲染 (&U)
├── ─────────────
└── 退出 (&Q)
```

- 右键点击显示菜单
- 双击图标显示/隐藏设置面板
- 布局变更时自动更新 tooltip 文字

---

### 4.9 可配置热键系统

**文件**：`src/app/AppConfig.h/cpp`、`src/app/Application.h/cpp`

所有热键均支持自定义修改。

#### HotkeyEntry

```cpp
struct HotkeyEntry {
    std::string name;     // 内部名（如 "preset_single"）
    std::string label;    // 显示名（如 "切换单屏"）
    int vk = 0;           // 虚拟键码（VK_1=0x31, VK_F1=0x70 等）
    bool ctrl = true;     // Ctrl 修饰键
    bool alt = true;      // Alt 修饰键
    bool shift = false;   // Shift 修饰键
};
```

#### 默认热键

| 功能 | 默认绑定 | action name | VK 码 |
|---|---|---|---|
| 切换单屏 | Ctrl+Alt+1 | preset_single | 0x31 |
| 切换双屏 | Ctrl+Alt+2 | preset_dual | 0x32 |
| 切换三屏 | Ctrl+Alt+3 | preset_triple | 0x33 |
| 切换五屏 | Ctrl+Alt+5 | preset_five | 0x35 |
| 切换 IMU | Ctrl+Alt+I | toggle_imu | 0x49 |
| 重置视角+漂移校正 | Ctrl+Alt+R | reset_view | 0x52 |
| 截图 | Ctrl+Alt+S | screenshot | 0x53 |
| 退出 | Ctrl+Alt+Q | quit | 0x51 |
| 显示/隐藏面板 | F1 | toggle_gui | 0x70 |
| 显示/隐藏预览 | F3 | toggle_preview | 0x72 |
| 切换锁定模式 | F4 | toggle_headlock | 0x73 |
| 显示/隐藏 HUD | F5 | toggle_hud | 0x74 |
| 亮度增加 | Ctrl+Alt+Up | brightness_up | 0x26 |
| 亮度减少 | Ctrl+Alt+Down | brightness_down | 0x28 |

#### 注册流程

```
Application::RegisterConfigurableHotkeys()
    ├── 注销旧热键（UnregisterHotKey）
    ├── 遍历 hotkeys.entries
    │   ├── 构建修饰键掩码（MOD_CONTROL | MOD_ALT | MOD_SHIFT）
    │   ├── RegisterHotKey(hwnd, id, mod, vk)
    │   └── m_vkToAction[id] = name  （ID → action 映射）
    └── 完成
```

#### HandleAction 分发

```cpp
void Application::HandleAction(const std::string& action) {
    if (action == "preset_single")  SwitchLayout("single");
    else if (action == "preset_dual")   SwitchLayout("dual");
    // ...
    else if (action == "reset_view") {
        m_camera.ResetYawZero();           // 漂移校正
        m_camera.SetRotation(0, 0, 0);     // 重置视角
    }
    else if (action == "toggle_headlock") m_camera.ToggleMode();
    else if (action == "screenshot") TakeScreenshot();
    // ...
}
```

---

### 4.10 配置文件

**文件**：`config/default.json`、`src/app/AppConfig.h/cpp`

#### 配置结构

```json
{
  "version": 1,
  "imu": {
    "enabled": true,
    "smoothing": 0.15,
    "deadzone_deg": 0.5,
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
    "screens": [
      {
        "captureIndex": 1,
        "name": "左屏",
        "position": [-0.75, 0, -1.5],
        "rotation": [0, 30, 0],
        "size": [0.6, 0.3375],
        "curvature": 0,
        "customWidth": 0,
        "customHeight": 0
      }
    ]
  },
  "rendering": {
    "vsync": true,
    "showBorders": true,
    "borderColor": [0.3, 0.3, 0.3],
    "borderWidth": 0.005,
    "backgroundColor": [0.02, 0.02, 0.04],
    "sharpenEnabled": false,
    "sharpenStrength": 0.5
  },
  "hud": {
    "enabled": true,
    "hotkeyVk": 116,
    "showFps": true,
    "showLatency": true,
    "showImu": true,
    "showCapture": true,
    "showLayout": true
  },
  "hotkeys": [
    { "name": "preset_single",  "label": "切换单屏",      "vk": 49,  "ctrl": true,  "alt": true,  "shift": false },
    { "name": "toggle_headlock","label": "切换锁定模式",  "vk": 115, "ctrl": false, "alt": false, "shift": false }
  ]
}
```

#### 加载流程

```
AppConfig::Load(path)
    ├── 读取 JSON 文件
    ├── 解析 imu / displays / rendering / hud
    ├── 解析 hotkeys 数组 → HotkeyEntry 列表
    ├── 填充缺失的默认热键（CreateDefaults 补全）
    ├── 解析 layout.screens → ScreenConfig 列表
    └── LayoutEngine::LoadFromJson(path) → LayoutConfig
```

#### 保存流程

```
AppConfig::Save(path)
    ├── 构建 JSON 对象
    ├── imu / displays / rendering / hud
    ├── hotkeys → JSON 数组
    ├── layout.screens → JSON 数组
    └── 写入文件（2 空格缩进）
```

---

### 4.11 日志模块

**文件**：`src/utils/Log.h/cpp`

统一日志接口，支持三个输出渠道。

#### 接口

```cpp
// 日志级别
enum class LogLevel { Debug, Info, Warn, Error };

// 初始化（Application::Init 中调用）
void LogInit(const std::string& logDir = "logs");
void LogShutdown();

// 日志输出宏
#define LOG_DEBUG(fmt, ...) LogWrite(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LogWrite(LogLevel::Info,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LogWrite(LogLevel::Warn,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LogWrite(LogLevel::Error, fmt, ##__VA_ARGS__)

// 底层写入
void LogWrite(LogLevel level, const char* fmt, ...);
```

#### 输出渠道

| 渠道 | 说明 | 条件 |
|---|---|---|
| 控制台 | `AllocConsole()` 弹出的黑色窗口，实时显示 | 始终启用 |
| 调试器 | Visual Studio 输出面板（`OutputDebugString`） | 仅调试构建 |
| 日志文件 | `logs/nreal_YYYY-MM-DD.log`，按天滚动 | 始终启用 |

#### 日志格式

```
[14:32:05.123] [INFO ] AirIMU: 连接成功, HID 设备 VID=3318
[14:32:05.456] [INFO ] CaptureManager: 捕获源 #1 初始化完成 (1920x1080)
[14:32:06.789] [WARN ] DisplayCapture: 源 #2 帧获取超时, 使用前帧
[14:32:10.012] [ERROR] AirIMU: HID 读取失败, 连接断开
```

#### 级别控制

配置文件中控制最低输出级别：

```json
"logging": {
    "level": "info",
    "consoleLevel": "info",
    "fileLevel": "debug"
}
```

- `consoleLevel` — 控制台只显示重要信息，避免刷屏
- `fileLevel` — 文件记录更详细的日志，用于事后排查

#### 性能注意事项

- 渲染循环内使用 `LOG_DEBUG` 而非 `LOG_INFO`，生产时关闭 debug 输出
- 日志写入使用异步队列 + 单独写入线程，不阻塞渲染
- 每日自动清理 7 天前的日志文件

---

### 4.12 错误处理策略

#### 错误分类

| 类别 | 示例 | 处理方式 |
|---|---|---|
| 可恢复 | WCG 帧获取超时、IMU 临时断开 | 重试 + 使用前帧数据 |
| 需重建 | D3D 设备丢失、SwapChain 失效 | 销毁资源 → 重新初始化 |
| 致命 | D3D 设备创建失败、无可用显示器 | 弹窗报错 → 退出 |

#### D3D 设备丢失恢复

```cpp
// D3DRenderer::EndFrame 中检查 Present 返回值
HRESULT hr = m_glassesSwapChain->Present(vsync ? 1 : 0, 0);
if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
    LOG_ERROR("D3D 设备丢失, 错误码: 0x%08X", hr);
    HandleDeviceLost();  // 销毁所有 D3D 资源 → 重建
}
```

`HandleDeviceLost()` 流程：
```
1. 释放所有 ComPtr（RTV、SRV、纹理、着色器、SwapChain）
2. 重新创建 D3D Device + Context
3. 重新创建离屏 RT + SwapChain
4. 重新编译着色器
5. 重新初始化捕获（WCG FramePool 需要新设备）
6. 恢复渲染
```

#### WCG 捕获失败处理

```cpp
// CaptureManager::CaptureAll 中
auto frame = m_framePool->TryGetNextFrame();
if (!frame) {
    // 无新帧，使用前帧（不报错）
    return;
}
auto surface = frame->Surface();
if (!surface) {
    LOG_WARN("捕获源 #%d 表面无效", index);
    m_status[index].errorCount++;
    if (m_status[index].errorCount > 60) {  // 连续 60 帧失败
        LOG_ERROR("捕获源 #%d 持续失败, 尝试重建", index);
        ReinitCapture(index);
    }
}
```

#### IMU 数据过期检测

```cpp
// Application::Tick 中
ImuData imu = m_imu.GetLatest();
uint64_t now = GetTimestampUs();
if (now - imu.timestampUs > 100000) {  // > 100ms
    LOG_WARN("IMU 数据过期 (%.1fms), 使用最后一帧", 
             (now - imu.timestampUs) / 1000.0f);
    // 不停止渲染，继续用旧数据
}
```

---

### 4.13 配置版本迁移

**文件**：`src/app/AppConfig.h/cpp`

JSON 配置中的 `"version"` 字段用于版本管理。加载时自动迁移旧配置。

#### 迁移策略

```cpp
AppConfig AppConfig::Load(const std::string& path) {
    json j = ReadJson(path);
    int version = j.value("version", 1);
    
    // 逐版本迁移
    if (version < 2) MigrateV1ToV2(j);
    if (version < 3) MigrateV2ToV3(j);
    // ...
    
    return ParseFromJson(j);
}
```

#### 默认值填充

加载时，缺失的字段使用默认值，不报错：

```cpp
// V1 → V2：新增 P7 优化字段
void AppConfig::MigrateV1ToV2(json& j) {
    auto& r = j["rendering"];
    if (!r.contains("sharpenEnabled"))  r["sharpenEnabled"] = false;
    if (!r.contains("sharpenStrength")) r["sharpenStrength"] = 0.5;
    
    auto& imu = j["imu"];
    if (!imu.contains("brightness")) imu["brightness"] = 50;
    
    j["version"] = 2;
    LOG_INFO("配置文件从 V1 迁移到 V2");
}
```

#### 保存时保留版本

保存时始终写入最新版本号，下次加载不需要再迁移。

---

## 5. 空间锚定原理

### 5.1 World Lock（默认）

```
IMU 四元数 (250Hz)
    ↓
ImuFilter（平滑 + 死区 + 漂移校正）
    ↓
Camera.m_rotation = filtered
    ↓
ViewMatrix = LookAtLH(position, position + rotate(forward, rotation), rotate(up, rotation))
    ↓
每个屏幕:
    WorldMatrix = Scale × RotZ × RotX × RotY × Translate  （固定不变）
    WVP = WorldMatrix × ViewMatrix × ProjectionMatrix
    ProjectionMatrix = 52° FOV 透视投影
    ↓
屏幕锚定在空间中，转头看到不同方向
```

### 5.2 Head Lock

```
IMU 四元数 (250Hz)
    ↓
ImuFilter（平滑 + 死区 + 漂移校正）
    ↓
Camera.m_rotation = GetHeadLockDelta(filtered)
    // delta = inverse(locked_quat) × current_quat
    // 即从锁定点开始的增量旋转
    ↓
ViewMatrix = 基于增量旋转
    ↓
屏幕始终跟随头部，保持相对位置不变
```

### 5.3 漂移校正

```
问题：IMU yaw 角随时间缓慢漂移（无外部参考）

解决：
1. 手动校正（Ctrl+Alt+R）：
   记录当前 yaw 为零点偏移 → 后续所有 yaw 值减去此偏移

2. 自动补偿（配置 autoDriftCompensation）：
   低通滤波检测缓慢变化 → 累积到偏移量 → 衰减

应用：
UpdateQuaternion 中 → ApplyYawCorrection
    → 提取 yaw → 加偏移 → 用新 yaw 重构四元数
```

---

## 6. 渲染管线详解

```
┌─────────────────────────────────────────────────────────┐
│                    BeginFrame()                           │
│  ├── ClearRenderTargetView（背景色）                      │
│  ├── ClearDepthStencilView                               │
│  ├── OMSetRenderTargets → 离屏 RT + 深度缓冲             │
│  ├── RSSetViewports → 场景尺寸                           │
│  └── 绑定场景着色器（VS + PS + Sampler + CB）             │
├─────────────────────────────────────────────────────────┤
│  DrawScreen() × N                                        │
│  ├── UpdateSubresource → 常量缓冲（WVP + 边框）          │
│  ├── 恢复场景着色器（Blit 可能覆盖了）                    │
│  ├── OMSetRenderTargets → 离屏 RT                        │
│  ├── PSSetShaderResources → 屏幕纹理 SRV                 │
│  ├── 选择 quad（曲面 m_screenQuads[i] / 平面 m_sharedQuad）│
│  └── DrawIndexed                                         │
├─────────────────────────────────────────────────────────┤
│  GUI 叠加                                                │
│  ├── ImGui::NewFrame → 绘制设置面板窗口                  │
│  └── ImGui::Render → 渲染到离屏 RT                       │
├─────────────────────────────────────────────────────────┤
│  EndFrame()                                              │
│  ├── Blit 离屏纹理 → 眼镜 back buffer → Present          │
│  └── Blit 离屏纹理 → 预览 back buffer（不 Present）       │
├─────────────────────────────────────────────────────────┤
│  预览 HUD                                                │
│  ├── ImGui（独立上下文）→ 渲染到预览 back buffer          │
│  └── PresentPreview()                                    │
└─────────────────────────────────────────────────────────┘
```

---

## 7. 主循环

**文件**：`src/main.cpp`、`src/app/Application.h/cpp`

### main.cpp

```cpp
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    AllocConsole();  // 日志控制台
    Application app;
    if (!app.Init()) { MessageBox(...); return 1; }

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
        } else {
            app.Tick();
        }
    }
    app.Shutdown();
}
```

### Application::Init 流程

```
1. CoInitializeEx
2. LayoutPreset::RegisterPresets
3. AppConfig::Load("config/default.json")
4. 注册窗口类 + 创建主窗口（全屏到 Nreal Light 或窗口模式）
5. D3DRenderer::Init → 离屏 RT + 眼镜 SwapChain
6. D3DRenderer::InitScreenResources(N) → 每屏纹理+SRV
7. 为曲率>0 的屏幕创建 InitCurvedScreen
8. D3DRenderer::InitPreview → 预览 SwapChain
9. SettingsGUI::Init + InitPreviewHud
10. CaptureManager::Init → 桌面捕获
11. AirIMU::Start（如启用）
12. LayoutEngine::UpdateWorldMatrices
13. Camera::SetPosition(viewerOffset)
14. RegisterConfigurableHotkeys
15. CreateTrayIcon
```

### Application::Tick 流程

```
1. 性能统计（FPS / 帧延迟）
2. Windows 消息处理
3. 总开关检查（暂停时 Sleep(16) 返回）
4. IMU → Camera::UpdateFromIMU
5. CaptureManager::CaptureAll
6. for each screen: UpdateTexture(i, tex)  // 捕获 → 纹理
7. D3DRenderer::BeginFrame
8. for each screen: DrawScreen(worldMatrix, viewProj, SRV, i)
9. SettingsGUI::NewFrame + Render  // GUI 叠加
10. D3DRenderer::EndFrame  // Blit → 眼镜 + 预览
11. SettingsGUI::RenderPreviewHud + PresentPreview  // HUD
12. GUI 数据更新（FPS/IMU/模式/计数）
```

---

## 8. 快捷键

| 快捷键 | 功能 | action name |
|---|---|---|
| F1 | 显示/隐藏设置面板 | toggle_gui |
| F3 | 显示/隐藏预览窗口 | toggle_preview |
| F4 | 切换 Head Lock / World Lock | toggle_headlock |
| F5 | 显示/隐藏 HUD | toggle_hud |
| Ctrl+Alt+1 | 单屏模式 | preset_single |
| Ctrl+Alt+2 | 双屏模式 | preset_dual |
| Ctrl+Alt+3 | 三屏模式 | preset_triple |
| Ctrl+Alt+5 | 五屏弧形 | preset_five |
| Ctrl+Alt+I | 切换 IMU | toggle_imu |
| Ctrl+Alt+R | 重置视角 + 漂移校正 | reset_view |
| Ctrl+Alt+S | 截图（BMP） | screenshot |
| Ctrl+Alt+Q | 退出程序 | quit |
| Ctrl+Alt+↑ | 亮度增加 | brightness_up |
| Ctrl+Alt+↓ | 亮度减少 | brightness_down |

所有热键可在 GUI 中自定义修改，支持冲突检测。

---

## 9. 快速开始

```bash
# 1. 确认硬件
#    - Nreal Light 已连接 PC (USB-C)
#    - Windows 识别为 "light" 显示器
#    - 设备管理器有 VID_3318 的 HID 设备

# 2. 安装 Virtual Display Driver
#    参考: https://github.com/19980202yyq/Virtual-Display-Driver

# 3. 下载 ImGui
cd src/gui && bash download_imgui.sh && cd ../..

# 4. 放置依赖到 lib/
#    - AirAPI_Windows.dll + .lib + .h
#    - hidapi.dll
#    - json.hpp (nlohmann)
#    - VirtualDesktopAccessor.dll（可选）

# 5. 构建
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 6. 运行（管理员权限）
NrealSpatialDisplay.exe
```

### 运行后

1. 程序自动查找 Nreal Light 显示器，找不到则进入窗口开发模式
2. 系统托盘出现图标，右键可切换布局
3. 按 F1 打开设置面板调整参数
4. 按 F3 打开预览窗口查看眼镜画面
5. 将应用窗口拖到虚拟显示器上，眼镜中看到多屏空间布局
6. 转头查看不同方向的屏幕
7. 按 F4 切换 Head Lock 模式
8. 右下角托盘图标可快速切换布局

---

## 10. 构建详解

### 前置条件

- Windows 10/11（1803+，WCG API 要求）
- Visual Studio 2022（或支持 C++17 的版本）
- NVIDIA 显卡（驱动 ≥ 530，Studio 驱动推荐）
- Nreal Light 通过 USB-C 连接
- Virtual Display Driver 已安装

### CMake 构建

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### HLSL 着色器编译

场景着色器（`screen_vs.hlsl` / `screen_ps.hlsl`）需要在 CMake 中配置编译为 `.cso` 文件：

```cmake
# CMakeLists.txt 中添加
set(HLSL_SHADER_FILES
    src/render/shaders/screen_vs.hlsl
    src/render/shaders/screen_ps.hlsl
)

foreach(HLSL_FILE ${HLSL_SHADER_FILES})
    get_filename_component(HLSL_NAME ${HLSL_FILE} NAME_WE)
    set(CSO_OUTPUT ${CMAKE_BINARY_DIR}/${HLSL_NAME}.cso)
    
    # VS 顶点着色器
    if(HLSL_NAME MATCHES "_vs$")
        add_custom_command(
            OUTPUT ${CSO_OUTPUT}
            COMMAND dxc.exe -T vs_5_0 -E VSMain -Fo ${CSO_OUTPUT} ${HLSL_FILE}
            DEPENDS ${HLSL_FILE}
        )
    # PS 像素着色器
    else()
        add_custom_command(
            OUTPUT ${CSO_OUTPUT}
            COMMAND dxc.exe -T ps_5_0 -E PSMain -Fo ${CSO_OUTPUT} ${HLSL_FILE}
            DEPENDS ${HLSL_FILE}
        )
    endif()
    
    list(APPEND CSO_OUTPUTS ${CSO_OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${CSO_OUTPUTS})
add_dependencies(NrealSpatialDisplay Shaders)
```

**注意**：Blit 着色器是运行时从内联 HLSL 字符串通过 `D3DCompile` 编译的，不需要 `.cso` 文件，也不需要 CMake 配置。

---

## 11. 开发阶段

| 阶段 | 内容 | 验证标准 | 状态 |
|---|---|---|---|
| P0 | AirAPI + IMU 读取 | 控制台打印 yaw/pitch/roll | ✅ |
| P1 | 单屏捕获 + D3D 渲染 | 眼镜中看到虚拟显示器 | ✅ |
| P2 | IMU → Camera → 空间锚定 | 转头时屏幕固定 | ✅ |
| P3 | 多屏 + 布局引擎 | 3 屏环绕 | ✅ |
| P4 | GUI + 热键 + 托盘 | 中文 GUI 全参数可调 | ✅ |
| P5 | 预览 + HUD + 截图 | 预览窗口 + 性能叠加 | ✅ |
| P6 | 漂移校正 + Head Lock + 吸附 | 长时间使用稳定 | ✅ |
| P7 | 体验优化（见第 13 节） | 断连恢复 < 3s、端到端延迟 < 20ms | 🔲 |

---

## 12. 竞品对比

| 功能 | GingerXR | VertoXR | NrealSpatialDisplay |
|---|---|---|---|
| 有线连接 | ✅ | ✅ | ✅ |
| 多虚拟屏 | 最多 5 屏 | 多屏 | 5 屏 |
| 空间锚定 | ✅ | ✅ | ✅ |
| 曲面屏 | ✅ | ✅ | ✅ |
| IMU 漂移校正 | ✅ | ✅ | ✅ 手动+自动 |
| Head Lock | ✅ | ✅ | ✅ |
| 屏幕吸附对齐 | ✅ | ✅ | ✅ |
| 每屏独立分辨率 | ✅ | ✅ | ✅ |
| 性能 HUD | ✅ | ✅ | ✅ |
| 截图 | ✅ | ✅ | ✅ BMP |
| 系统托盘 | ✅ | ✅ | ✅ |
| 中文 GUI | ❌ | ❌ | ✅ |
| 热键冲突检测 | ❌ | ❌ | ✅ |
| 开源 | ❌ | ❌ | ✅ |
| 无线串流 | ✅ | ✅ | ❌ |
| 屏幕透明度 | ✅ | ✅ | ❌ |
| 断连自动恢复 | ✅ | ✅ | ✅（P7） |
| 端到端延迟优化 | ✅ | ✅ | ✅（P7） |
| 亮度控制（GUI） | ✅ | ✅ | ✅（P7） |
| 文本锐化 | ❌ | ❌ | ✅（P7） |
| 电源管理 | ❌ | ❌ | ✅（P7） |

---

## 13. 体验优化（P7）

### 13.1 断连自动恢复

**问题**：眼镜 USB 松动、电脑休眠唤醒后，程序需要重启才能恢复。

**方案**：在 Application 层增加连接状态机。

```cpp
enum class ConnectionState {
    Connected,          // 正常运行
    Disconnected,       // 检测到断开
    Reconnecting,       // 正在重试
    Recovered           // 重连成功，恢复渲染
};
```

**实现**：
- IMU 层：`AirIMU::PollLoop` 检测到 HID 读取失败 → 设置 `m_connected = false` → 回调通知 Application
- 捕获层：`CaptureManager::CaptureAll` 检测到 `AcquireNextFrame` 返回 `DXGI_ERROR_ACCESS_LOST` → 标记该源断开
- Application：收到断开通知 → 停止渲染循环 → 每 1 秒重试 IMU 连接 + 重新初始化捕获
- 恢复后：重建 SwapChain（如需要）→ 恢复渲染 → HUD 显示 "已重连"

**验证**：拔掉眼镜 USB → 等待 3 秒 → 重新插入 → 3 秒内自动恢复画面。

### 13.2 端到端延迟优化

**问题**：IMU 数据到画面更新的总延迟未量化，AR 眼镜上延迟 > 20ms 会产生明显拖拽感。

**延迟链路**：
```
IMU 采样 (4ms) → 轮询获取 (0ms) → 滤波 (0.1ms) → 相机更新 (0.1ms)
→ 桌面捕获 (8-16ms) → 渲染 (2-4ms) → Blit + Present (1-2ms)
总延迟: 15-27ms
```

**优化方案**：

| 环节 | 当前 | 优化后 | 方法 |
|---|---|---|---|
| 桌面捕获 | 同步等待帧 | 异步 + 前帧复用 | WCG FramePool 回调独立线程，渲染时用最新可用帧 |
| IMU → 渲染 | 串行 | 并行流水线 | IMU 数据双缓冲，渲染线程不阻塞 IMU 线程 |
| Blit | 单次 Blit | 合并到场景 Pass | 最后一个屏幕渲染完直接输出到 SwapChain，省一次全屏 Blit |
| VSync | 始终开启 | 可选关闭 | 配置项 `rendering.vsync`，低延迟场景关闭 |

**HUD 增强**：在预览 HUD 中增加端到端延迟显示：
```
端到端: 18.3ms  (IMU→画面)
捕获: 12.1ms   渲染: 3.2ms
```

### 13.3 虚拟显示器状态监控

**问题**：虚拟显示器不存在或分辨率不对时，用户需手动去设备管理器排查。

**方案**：`CaptureManager::Init` 时检查每个目标显示器：
- 枚举系统所有显示器（`EnumDisplayDevices`）
- 验证虚拟显示器是否存在
- 验证分辨率是否匹配配置
- 不匹配时在 HUD 和托盘 tooltip 中显示警告

```cpp
struct DisplayStatus {
    bool exists;           // 显示器是否存在
    bool resolutionMatch;  // 分辨率是否匹配
    uint32_t actualWidth;
    uint32_t actualHeight;
    std::string deviceName;
};

std::vector<DisplayStatus> CaptureManager::CheckDisplayStatus(
    ID3D11Device* device, const std::vector<int>& indices);
```

**托盘提示**：存在异常时托盘图标加 ⚠ 标记，tooltip 显示具体问题。

### 13.4 文本锐化

**问题**：1920×1080 塞在 52° FOV 里，小字号文本模糊。

**方案**：在渲染管线中增加可选的锐化后处理 Pass。

**实现**：在 Blit 之前插入 Unsharp Mask：
```
离屏纹理 → 锐化 Pass（可选）→ Blit → SwapChain
```

锐化着色器（内联 HLSL 编译，与 Blit 着色器相同方式）：
```hlsl
cbuffer SharpenCB : register(b0) {
    float g_strength;     // 锐化强度（0.0~2.0，默认 0.5）
    float2 g_texelSize;   // 1.0 / 纹理尺寸
};

float4 PSSharpen(PS_IN input) : SV_TARGET {
    float4 center = g_texture.Sample(g_sampler, input.uv);
    float4 top    = g_texture.Sample(g_sampler, input.uv + float2(0, -g_texelSize.y));
    float4 bottom = g_texture.Sample(g_sampler, input.uv + float2(0,  g_texelSize.y));
    float4 left   = g_texture.Sample(g_sampler, input.uv + float2(-g_texelSize.x, 0));
    float4 right  = g_texture.Sample(g_sampler, input.uv + float2( g_texelSize.x, 0));
    float4 blur = (top + bottom + left + right) * 0.25;
    return center + (center - blur) * g_strength;
}
```

**配置项**：
```json
"rendering": {
    "sharpenEnabled": false,
    "sharpenStrength": 0.5
}
```

**GUI**：渲染标签页增加 "文本锐化" 开关和强度滑块。

### 13.5 亮度控制

**问题**：`AirAPI::GetBrightness` 已实现但未暴露到 GUI。

**方案**：
- 增加 `AirAPI::SetBrightness(int level)`（如 AirAPI 支持写入）
- 设置面板 IMU 标签页增加亮度滑块（0-100）
- 如 AirAPI 不支持写入，则仅显示当前亮度只读值

```cpp
// AirIMU.h
int GetBrightness();                      // 已有
bool SetBrightness(int level);            // 新增（如支持）
```

**GUI**：IMU 标签页底部增加亮度控件：
```
亮度: [====|========] 65%
```

### 13.6 电源管理

**问题**：暂停渲染时，捕获和 IMU 轮询仍在运行，浪费 CPU/GPU 资源和笔记本电量。

**方案**：暂停时分层停止：

| 组件 | 暂停时行为 | 恢复时行为 |
|---|---|---|
| 渲染循环 | 跳过，Sleep(100) | 恢复正常 Tick |
| WCG FramePool | `Pause()` 暂停回调 | `Resume()` 恢复 |
| 捕获线程 | 暂停 | 恢复 |
| IMU 轮询 | 降频到 10Hz（省 CPU） | 恢复 250Hz |
| SwapChain | 不 Present | 恢复 Present |

```cpp
void Application::SetPaused(bool paused) {
    m_paused = paused;
    if (paused) {
        m_captureManager.Pause();       // WCG FramePool::Pause()
        m_imu.SetPollRate(10);          // 降频
    } else {
        m_captureManager.Resume();      // WCG FramePool::Resume()
        m_imu.SetPollRate(250);         // 恢复
    }
}
```

### 13.7 启动速度优化

**问题**：Init 流程 15 步串行，启动慢。

**方案**：分两阶段启动：

**阶段 1（同步，< 500ms）**：托盘图标出现，程序可用
```
1. CoInitializeEx
2. AppConfig::Load
3. CreateTrayIcon          ← 用户立即看到托盘图标
```

**阶段 2（异步，后台）**：渲染管线初始化
```
4. D3DRenderer::Init
5. InitScreenResources
6. InitPreview
7. SettingsGUI::Init
8. CaptureManager::Init
9. AirIMU::Start           ← 不阻塞主线程
10. LayoutEngine + Camera
11. RegisterHotkeys
12. 首帧渲染
```

**托盘状态**：初始化期间 tooltip 显示 "正在启动..."，完成后显示布局信息。

**错误处理**：异步阶段失败时，托盘图标变灰 + tooltip 显示错误原因（如 "未找到眼镜显示器"）。

---

## 14. 故障排查

### GPU 兼容性（NVIDIA）

本项目在 NVIDIA 独显上开发和测试，以下为已知注意事项：

| 项目 | 说明 |
|---|---|
| D3D 设备创建 | 优先使用硬件设备，NVIDIA 驱动版本 ≥ 530 推荐 |
| D3DCompile | 内联 HLSL 编译依赖 `d3dcompiler_47.dll`，Win10/11 自带 |
| WCG 捕获 | NVIDIA 显卡支持良好，捕获延迟通常 < 10ms |
| 离屏渲染 | 大尺寸离屏 RT（如 5 屏 1920×1080）显存占用约 120MB |
| 休眠恢复 | NVIDIA 驱动休眠后可能触发 `DXGI_ERROR_DEVICE_REMOVED`，需走设备丢失恢复流程 |
| 驱动更新 | 更新驱动后首次运行可能触发一次设备丢失，属正常现象 |

**推荐驱动版本**：≥ 530（Studio 驱动优先，比 Game Ready 更稳定）

**NVIDIA 控制面板设置**（可选优化）：
- 电源管理模式 → 最高性能优先
- 垂直同步 → 由应用程序控制（避免驱动层强制 VSync）

### 眼镜黑屏

| 检查项 | 排查方法 |
|---|---|
| USB 连接 | 设备管理器查看是否有 VID_3318 的 HID 设备 |
| 显示器识别 | Windows 显示设置中是否出现 "light" 显示器 |
| SwapChain 创建 | 日志中搜索 `ERROR` + `SwapChain` |
| 分辨率匹配 | 确认虚拟显示器分辨率 = 1920×1080 |
| 程序是否在渲染 | 托盘图标是否显示 "暂停渲染"，按 F1 检查总开关 |

### 画面卡顿

| 检查项 | 排查方法 |
|---|---|
| GPU 占用 | 任务管理器 → 性能 → GPU，确认占用率 |
| VSync 冲突 | 设置面板 → 渲染 → 关闭 VSync 测试 |
| 捕获帧率 | HUD 中查看捕获源数量和延迟 |
| 曲面屏开销 | 尝试切换到平面布局（single/dual）对比 |
| 后台程序 | 关闭其他 GPU 密集型程序（游戏、视频渲染） |

### IMU 无数据 / 头部转动无反应

| 检查项 | 排查方法 |
|---|---|
| HID 设备 | 设备管理器 → 人体学输入设备 → 查找 VID_3318 |
| AirAPI 连接 | 日志中搜索 `AirIMU` 相关信息 |
| IMU 开关 | 设置面板 → IMU 标签页 → 确认 IMU 已启用 |
| 数据过期 | HUD 中查看 IMU 状态，P/Y/R 数值是否变化 |
| USB 供电 | 尝试换一个 USB-C 口，排除供电不足 |

### 屏幕位置错乱

| 检查项 | 排查方法 |
|---|---|
| 配置文件 | 检查 `config/default.json` 中 `layout.screens` 是否正确 |
| 虚拟显示器编号 | `captureIndex` 是否与实际虚拟显示器对应 |
| 吸附对齐 | 设置面板 → 屏幕编辑 → 点击 "重置布局" |
| 观察偏移 | 检查 `viewerOffset` 是否被意外修改 |

### 热键无响应

| 检查项 | 排查方法 |
|---|---|
| 快捷键冲突 | 设置面板 → 快捷键标签页 → 查看是否有 ⚠ 冲突标记 |
| 被其他程序占用 | 关闭其他全局热键程序（如 PowerToys、AutoHotkey） |
| RegisterHotKey 失败 | 日志中搜索 `RegisterHotKey` 或 `热键` |
| 管理员权限 | 部分热键需要程序以管理员权限运行 |

### 断连后无法恢复

| 检查项 | 排查方法 |
|---|---|
| 自动重连是否启用 | 检查 P7 断连恢复功能是否已实现 |
| USB 重新连接 | 拔插 USB 后等待 3 秒，观察日志 |
| SwapChain 重建 | 日志中搜索 `设备丢失` 或 `DeviceLost` |
| 手动重启 | 如自动恢复失败，退出后重新启动程序 |

### 日志文件位置

```
程序目录/
├── logs/
│   ├── nreal_2026-04-29.log    # 当天日志
│   ├── nreal_2026-04-28.log    # 昨天
│   └── ...                     # 自动保留 7 天
└── config/
    └── default.json
```

快速排查：打开当天日志文件，搜索 `ERROR` 和 `WARN` 级别条目。
