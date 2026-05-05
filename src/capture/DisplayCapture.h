#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <windows.graphics.capture.h>
#include <windows.graphics.directx.h>
#include <windows.graphics.directx.direct3d11.h>

using Microsoft::WRL::ComPtr;
using namespace ABI::Windows::Graphics::Capture;
using namespace ABI::Windows::Graphics::DirectX;
using namespace ABI::Windows::Graphics::DirectX::Direct3D11;

class DisplayCapture {
public:
    DisplayCapture() = default;
    ~DisplayCapture() { Shutdown(); }

    bool Init(ID3D11Device* device, int displayIndex);
    void Shutdown();
    bool CaptureFrame(ID3D11DeviceContext* ctx, ID3D11Texture2D** outTexture);
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool IsInitialized() const { return m_initialized; }
    bool IsWGCActive() const { return m_wgcActive; }

    static std::vector<int> EnumerateDisplays();

private:
    bool m_initialized = false;
    int m_displayIndex = -1;
    uint32_t m_width = 1920;
    uint32_t m_height = 1080;

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Texture2D> m_cachedTexture;
    ComPtr<ID3D11Texture2D> m_stagingTexture;
    bool m_hasFrame = false;

    // GDI捕获
    HDC m_hdcScreen = nullptr;
    HDC m_hdcMem = nullptr;
    HBITMAP m_hBitmap = nullptr;
    BITMAPINFO m_bmi = {};
    BYTE* m_dibBits = nullptr;

    // WGC捕获
    bool m_wgcActive = false;
    ComPtr<IDirect3D11CaptureFramePool> m_framePool;
    ComPtr<IGraphicsCaptureSession> m_captureSession;
    std::mutex m_wgcMutex;
    ComPtr<ID3D11Texture2D> m_wgcLatestFrame;
    bool m_wgcFrameReady = false;

    bool InitGDI(ID3D11Device* device);
    bool InitWGC(int displayIndex);
    bool UpdateWGCFrame(ID3D11DeviceContext* ctx);
    bool UpdateGDIFrame(ID3D11DeviceContext* ctx);
    bool UpdateSimulatedFrame(ID3D11DeviceContext* ctx);

    ComPtr<IDXGIDevice> m_dxgiDevice;
};
