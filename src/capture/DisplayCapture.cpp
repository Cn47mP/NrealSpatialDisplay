// DisplayCapture.cpp - Windows Graphics Capture API 实现
// 使用 WinRT ABI 调用，无需 C++/WinRT 头文件依赖
#include "DisplayCapture.h"
#include "../utils/Log.h"
#include "../utils/ComHelper.h"

#include <dxgi1_2.h>
#include <d3d11.h>
#include <wrl.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dxgi.h>

using namespace Microsoft::WRL;

// ─── WinRT 激活工厂（不需要 #pragma comment，运行时动态解析） ───

// Windows.Graphics.Capture.GraphicsCaptureItem 静态工厂
// 通过 IGraphicsCaptureItemInterop 创建针对 HMONITOR 的捕获项

static HMODULE s_winrtModule = nullptr;

typedef HRESULT(WINAPI *PFN_RoGetActivationFactory)(
    HSTRING classId, REFIID iid, void** factory);
typedef HRESULT(WINAPI *PFN_WindowsCreateString)(
    LPCWSTR src, UINT32 len, HSTRING* out);
typedef HRESULT(WINAPI *PFN_WindowsDeleteString)(HSTRING h);

static PFN_RoGetActivationFactory pRoGetActivationFactory = nullptr;
static PFN_WindowsCreateString pWindowsCreateString = nullptr;
static PFN_WindowsDeleteString pWindowsDeleteString = nullptr;

static bool EnsureWinRT() {
    if (pRoGetActivationFactory) return true;
    s_winrtModule = LoadLibraryW(L"combase.dll");
    if (!s_winrtModule) return false;
    pRoGetActivationFactory = (PFN_RoGetActivationFactory)
        GetProcAddress(s_winrtModule, "RoGetActivationFactory");
    pWindowsCreateString = (PFN_WindowsCreateString)
        GetProcAddress(s_winrtModule, "WindowsCreateString");
    pWindowsDeleteString = (PFN_WindowsDeleteString)
        GetProcAddress(s_winrtModule, "WindowsDeleteString");
    return pRoGetActivationFactory && pWindowsCreateString && pWindowsDeleteString;
}

// ─── IDirect3DDxgiInterfaceAccess → 用于从 WinRT 对象拿回 D3D 纹理 ───

MIDL_INTERFACE("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")
IDirect3DDxgiInterfaceAccess : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};

// ─── 辅助：通过 IDirect3DDevice 获取 ID3D11Device ───

static ComPtr<ID3D11Device> GetD3DDeviceFromRTDevice(IUnknown* rtDevice) {
    ComPtr<IDirect3DDxgiInterfaceAccess> dxgiAccess;
    HRESULT hr = rtDevice->QueryInterface(IID_PPV_ARGS(&dxgiAccess));
    if (FAILED(hr)) return nullptr;
    ComPtr<ID3D11Device> d3dDevice;
    hr = dxgiAccess->GetInterface(IID_PPV_ARGS(&d3dDevice));
    if (FAILED(hr)) return nullptr;
    return d3dDevice;
}

// ─── 辅助：创建 WinRT Direct3D 设备包装 ───

static ComPtr<IUnknown> CreateWinRTD3DDevice(ID3D11Device* d3dDevice) {
    // 通过 IDirect3DDxgiInterfaceAccess 反向创建
    // 或者使用 CreateDirect3D11DeviceFromDXGIDevice
    ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(hr)) return nullptr;

    // 尝试加载 d3d11.dll 中的 CreateDirect3D11DeviceFromDXGIDevice
    HMODULE d3d11 = GetModuleHandleW(L"d3d11.dll");
    if (!d3d11) d3d11 = LoadLibraryW(L"d3d11.dll");
    if (!d3d11) return nullptr;

    typedef HRESULT(WINAPI *PFN_CreateDXGIFactory2)(UINT, REFIID, void**);
    // 使用标准路径：DXGIDevice → WinRT device
    // CreateDirect3D11DeviceFromDXGIDevice 在 d3d11.dll 中
    typedef HRESULT(WINAPI *PFN_CreateFromDXGI)(IUnknown*, IUnknown**);
    auto pCreate = (PFN_CreateFromDXGI)GetProcAddress(d3d11, "CreateDirect3D11DeviceFromDXGIDevice");
    if (!pCreate) return nullptr;

    ComPtr<IUnknown> rtDevice;
    hr = pCreate(dxgiDevice.Get(), &rtDevice);
    if (FAILED(hr)) return nullptr;
    return rtDevice;
}

// ─── DisplayCapture 实现 ───

std::vector<int> DisplayCapture::EnumerateDisplays() {
    std::vector<int> displays;

    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory))) {
        return displays;
    }

    int adapterIndex = 0;
    ComPtr<IDXGIAdapter> adapter;
    while (SUCCEEDED(factory->EnumAdapters(adapterIndex, &adapter))) {
        ComPtr<IDXGIOutput> output;
        int outputIndex = 0;
        while (SUCCEEDED(adapter->EnumOutputs(outputIndex, &output))) {
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(output->GetDesc(&desc))) {
                if (desc.AttachedToDesktop) {
                    displays.push_back(adapterIndex * 10 + outputIndex);
                    LOG_INFO("DisplayCapture: Display %d = adapter %d output %d (%dx%d)",
                             (int)displays.size()-1, adapterIndex, outputIndex,
                             desc.DesktopCoordinates.right - desc.DesktopCoordinates.left,
                             desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
                }
            }
            output.Reset();
            outputIndex++;
        }
        adapter.Reset();
        adapterIndex++;
    }

    LOG_INFO("DisplayCapture: Found %d displays", (int)displays.size());
    return displays;
}

bool DisplayCapture::Init(ID3D11Device* device, int displayIndex) {
    m_device = device;
    m_displayIndex = displayIndex;

    // 尝试 WGC 高性能捕获
    if (InitWGC(displayIndex)) {
        m_initialized = true;
        return true;
    }

    // WGC 失败，回退到 GDI
    LOG_INFO("DisplayCapture: WGC unavailable, falling back to GDI");
    return InitGDI(device);
}

bool DisplayCapture::InitGDI(ID3D11Device* device) {
    m_device = device;

    // 枚举显示器获取真实分辨率
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);
    DEVMODEW dm = {};
    if (EnumDisplayDevicesW(nullptr, m_displayIndex, &dd, 0) && EnumDisplaySettingsW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
        m_width = dm.dmPelsWidth;
        m_height = dm.dmPelsHeight;
    }

    // 创建默认纹理
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, &m_cachedTexture);
    if (FAILED(hr)) {
        LOG_ERROR("DisplayCapture: CreateTexture2D failed: 0x%08X", hr);
        return false;
    }

    // 创建staging纹理用于CPU写入
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateTexture2D(&desc, nullptr, &m_stagingTexture);
    if (FAILED(hr)) {
        LOG_ERROR("DisplayCapture: Create staging texture failed: 0x%08X", hr);
        return false;
    }

    // 初始化GDI捕获资源
    m_hdcScreen = GetDC(nullptr);
    m_hdcMem = CreateCompatibleDC(m_hdcScreen);
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biWidth = m_width;
    m_bmi.bmiHeader.biHeight = -m_height; // 顶部朝下
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32;
    m_bmi.bmiHeader.biCompression = BI_RGB;
    m_hBitmap = CreateDIBSection(m_hdcMem, &m_bmi, DIB_RGB_COLORS, (void**)&m_frameBuffer, nullptr, 0);
    SelectObject(m_hdcMem, m_hBitmap);

    m_frameBuffer.resize(m_width * m_height * 4);

    LOG_INFO("DisplayCapture: Source #%d using GDI capture (%ux%u)", m_displayIndex, m_width, m_height);

    m_initialized = true;
    return true;
}

bool DisplayCapture::InitWGC(int displayIndex) {
    if (!EnsureWinRT()) {
        LOG_WARN("DisplayCapture: WinRT runtime not available");
        return false;
    }

    // 枚举 HMONITOR
    struct MonitorInfo {
        HMONITOR hmon;
        RECT rect;
        int index;
    };
    std::vector<MonitorInfo> monitors;

    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT rect, LPARAM lp) -> BOOL {
        auto* vec = reinterpret_cast<std::vector<MonitorInfo>*>(lp);
        MONITORINFOEXW mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(hmon, &mi)) {
            vec->push_back({hmon, *rect, (int)vec->size()});
        }
        return TRUE;
    }, (LPARAM)&monitors);

    if (displayIndex < 0 || displayIndex >= (int)monitors.size()) {
        LOG_WARN("DisplayCapture: Display index %d out of range (have %d)", displayIndex, (int)monitors.size());
        return false;
    }

    HMONITOR targetMonitor = monitors[displayIndex].hmon;
    m_width = monitors[displayIndex].rect.right - monitors[displayIndex].rect.left;
    m_height = monitors[displayIndex].rect.bottom - monitors[displayIndex].rect.top;

    // 获取 IGraphicsCaptureItemInterop
    HSTRING hsClassName;
    HRESULT hr = pWindowsCreateString(
        L"Windows.Graphics.Capture.GraphicsCaptureItem",
        46, &hsClassName);
    if (FAILED(hr)) {
        LOG_ERROR("DisplayCapture: WindowsCreateString failed: 0x%08X", hr);
        return false;
    }

    ComPtr<IGraphicsCaptureItemInterop> interop;
    hr = pRoGetActivationFactory(hsClassName,
        IID_PPV_ARGS(&interop));
    pWindowsDeleteString(hsClassName);
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: IGraphicsCaptureItemInterop not available: 0x%08X", hr);
        return false;
    }

    // 创建针对显示器的捕获项
    ComPtr<IGraphicsCaptureItem> item;
    hr = interop->CreateForMonitor(targetMonitor, IID_PPV_ARGS(&item));
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: CreateForMonitor failed: 0x%08X", hr);
        return false;
    }

    // 获取 IDirect3D11CaptureFramePoolStatics
    HSTRING hsFramePool;
    pWindowsCreateString(
        L"Windows.Graphics.Capture.Direct3D11CaptureFramePool",
        53, &hsFramePool);

    ComPtr<IDirect3D11CaptureFramePoolStatics> framePoolStatics;
    hr = pRoGetActivationFactory(hsFramePool,
        IID_PPV_ARGS(&framePoolStatics));
    pWindowsDeleteString(hsFramePool);
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: FramePoolStatics not available: 0x%08X", hr);
        return false;
    }

    // 创建 WinRT D3D 设备
    ComPtr<IUnknown> rtDeviceUnknown = CreateWinRTD3DDevice(m_device.Get());
    if (!rtDeviceUnknown) {
        LOG_WARN("DisplayCapture: CreateWinRTD3DDevice failed");
        return false;
    }

    ComPtr<IDirect3DDevice> rtDevice;
    hr = rtDeviceUnknown.As(&rtDevice);
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: QI for IDirect3DDevice failed: 0x%08X", hr);
        return false;
    }

    // 创建帧池
    ComPtr<IDirect3D11CaptureFramePool> framePool;
    hr = framePoolStatics->Create(
        rtDevice.Get(),
        DirectXPixelFormat::DirectXPixelFormat_B8G8R8A8UIntNormalized,
        2,  // 双缓冲
        {static_cast<int32_t>(m_width), static_cast<int32_t>(m_height)},
        &framePool);
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: FramePool::Create failed: 0x%08X", hr);
        return false;
    }

    // 创建捕获会话
    ComPtr<IGraphicsCaptureSession> session;
    hr = framePool->CreateCaptureSession(item.Get(), &session);
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: CreateCaptureSession failed: 0x%08X", hr);
        return false;
    }

    // 启动捕获
    hr = session->StartCapture();
    if (FAILED(hr)) {
        LOG_WARN("DisplayCapture: StartCapture failed: 0x%08X", hr);
        return false;
    }

    // 创建输出纹理
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_cachedTexture);
    if (FAILED(hr)) {
        LOG_ERROR("DisplayCapture: CreateTexture2D for WGC failed: 0x%08X", hr);
        return false;
    }

    // 保存到成员变量
    m_framePool = framePool;
    m_captureSession = session;
    m_wgcActive = true;

    LOG_INFO("DisplayCapture: WGC capture started on monitor %d (%ux%u)", displayIndex, m_width, m_height);
    return true;
}

bool DisplayCapture::UpdateWGCFrame(ID3D11DeviceContext* ctx) {
    if (!m_wgcActive || !m_framePool) return false;

    ComPtr<IDirect3D11CaptureFrame> frame;
    HRESULT hr = m_framePool->TryGetNextFrame(&frame);
    if (FAILED(hr) || !frame) {
        return false;
    }

    // 从捕获帧获取纹理
    ComPtr<IDirect3DDxgiInterfaceAccess> access;
    hr = frame->QueryInterface(IID_PPV_ARGS(&access));
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID3D11Texture2D> frameTexture;
    hr = access->GetInterface(IID_PPV_ARGS(&frameTexture));
    if (FAILED(hr)) {
        return false;
    }

    // 拷贝到缓存纹理
    ctx->CopyResource(m_cachedTexture.Get(), frameTexture.Get());
    return true;
}

bool DisplayCapture::UpdateGDIFrame(ID3D11DeviceContext* ctx) {
    // 捕获屏幕到DIB段
    BitBlt(m_hdcMem, 0, 0, m_width, m_height, m_hdcScreen, 0, 0, SRCCOPY);

    // 拷贝到staging纹理
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(ctx->Map(m_stagingTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return false;
    }

    BYTE* dst = (BYTE*)mapped.pData;
    for (UINT y = 0; y < m_height; y++) {
        memcpy(dst + y * mapped.RowPitch, m_frameBuffer.data() + y * m_width * 4, m_width * 4);
    }

    ctx->Unmap(m_stagingTexture.Get(), 0);
    ctx->CopyResource(m_cachedTexture.Get(), m_stagingTexture.Get());
    return true;
}

void DisplayCapture::Shutdown() {
    // 清理 WGC 资源（ComPtr 析构自动释放，无需显式 Close）
    if (m_wgcActive) {
        m_captureSession.Reset();
        m_framePool.Reset();
        m_wgcLatestFrame.Reset();
        m_wgcActive = false;
    }

    // 清理 GDI 资源
    if (m_hBitmap) DeleteObject(m_hBitmap);
    if (m_hdcMem) DeleteDC(m_hdcMem);
    if (m_hdcScreen) ReleaseDC(nullptr, m_hdcScreen);

    m_cachedTexture.Reset();
    m_stagingTexture.Reset();
    m_hasFrame = false;
    m_initialized = false;
    m_dxgiDevice.Reset();
}

bool DisplayCapture::CaptureFrame(ID3D11DeviceContext* ctx, ID3D11Texture2D** outTexture) {
    if (!m_initialized || !outTexture) return false;

    // 优先使用 WGC 高性能捕获
    if (m_wgcActive) {
        if (UpdateWGCFrame(ctx)) {
            m_hasFrame = true;
        }
    }
    // 回退到 GDI 捕获
    else if (UpdateGDIFrame(ctx)) {
        m_hasFrame = true;
    } else {
        // 捕获失败时使用模拟帧
        UpdateSimulatedFrame(ctx);
        m_hasFrame = true;
    }

    *outTexture = m_cachedTexture.Get();
    if (*outTexture) {
        (*outTexture)->AddRef();
    }
    return true;
}

bool DisplayCapture::UpdateSimulatedFrame(ID3D11DeviceContext* ctx) {
    static int frameCounter = 0;
    frameCounter++;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(ctx->Map(m_cachedTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return false;
    }

    BYTE* pixels = (BYTE*)mapped.pData;
    int pitch = mapped.RowPitch;
    float time = (float)frameCounter * 0.016f;

    for (uint32_t y = 0; y < m_height; y++) {
        for (uint32_t x = 0; x < m_width; x++) {
            int idx = (y * pitch) + (x * 4);
            float nx = (float)x / m_width - 0.5f;
            float ny = (float)y / m_height - 0.5f;
            float wave1 = sinf(nx * 8.0f + time);
            float wave2 = cosf(ny * 6.0f + time * 0.8f);
            float pattern = wave1 * wave2 * 0.5f + 0.5f;

            pixels[idx + 0] = (BYTE)(pattern * 80 + 30);
            pixels[idx + 1] = (BYTE)(pattern * 120 + 50);
            pixels[idx + 2] = (BYTE)(pattern * 180 + 40);
            pixels[idx + 3] = 255;
        }
    }

    ctx->Unmap(m_cachedTexture.Get(), 0);
    return true;
}
