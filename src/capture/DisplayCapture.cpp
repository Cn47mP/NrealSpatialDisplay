#include "DisplayCapture.h"
#include "../utils/Log.h"
#include "../utils/ComHelper.h"

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
                }
            }
            output.Reset();
            outputIndex++;
        }
        adapter.Reset();
        adapterIndex++;
    }

    LOG_INFO("DisplayCapture: Found %d displays", displays.size());
    return displays;
}

bool DisplayCapture::Init(ID3D11Device* device, int displayIndex) {
    m_device = device;
    m_displayIndex = displayIndex;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HR_CHECK_RET(device->CreateTexture2D(&desc, nullptr, &m_cachedTexture));

    LOG_INFO("DisplayCapture: Source #%d using animated pattern (%ux%u)", displayIndex, m_width, m_height);
    m_initialized = true;
    return true;
}

void DisplayCapture::Shutdown() {
    m_cachedTexture.Reset();
    m_hasFrame = false;
    m_initialized = false;
    m_dxgiDevice.Reset();
}

bool DisplayCapture::CaptureFrame(ID3D11DeviceContext* ctx, ID3D11Texture2D** outTexture) {
    if (!m_initialized || !outTexture) return false;

    if (!UpdateSimulatedFrame(ctx)) {
        return false;
    }

    *outTexture = m_cachedTexture.Get();
    if (*outTexture) {
        (*outTexture)->AddRef();
    }
    m_hasFrame = true;
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