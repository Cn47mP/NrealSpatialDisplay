#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <functional>
#include <windows.graphics.capture.h>

using Microsoft::WRL::ComPtr;

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

    static std::vector<int> EnumerateDisplays();

private:
    bool m_initialized = false;
    int m_displayIndex = -1;
    uint32_t m_width = 1920;
    uint32_t m_height = 1080;

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Texture2D> m_cachedTexture;
    bool m_hasFrame = false;

    // WGC 捕获
    bool InitWGC(int displayIndex);
    bool m_wgcActive = false;
    ComPtr<IDirect3D11CaptureFramePool> m_framePool;
    ComPtr<IGraphicsCaptureSession> m_captureSession;

    // 模拟帧备用
    bool UpdateSimulatedFrame(ID3D11DeviceContext* ctx);

    ComPtr<IDXGIDevice> m_dxgiDevice;
};
