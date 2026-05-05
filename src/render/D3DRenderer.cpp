#include "D3DRenderer.h"
#include "../utils/Log.h"
#include "../utils/ComHelper.h"
#include <d3dcompiler.h>

bool D3DRenderer::CreateDevice() {
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HR_CHECK_RET(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &m_device,
        &featureLevel,
        &m_context
    ));

    LOG_INFO("D3DRenderer: Device created, feature level %d", featureLevel);
    return true;
}

bool D3DRenderer::CreateSwapChain(HWND hwnd, UINT w, UINT h, ComPtr<IDXGISwapChain1>& sc, ComPtr<ID3D11RenderTargetView>& rtv) {
    LOG_INFO("CreateSwapChain: getting DXGI factory");
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = m_device.As(&dxgiDevice);
    LOG_INFO("CreateSwapChain: device->dxgi hr=%x", hr);
    if (FAILED(hr)) return false;
    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    LOG_INFO("CreateSwapChain: get adapter hr=%x", hr);
    if (FAILED(hr)) return false;
    hr = adapter->GetParent(__uuidof(IDXGIFactory2), &factory);
    LOG_INFO("CreateSwapChain: get parent factory hr=%x", hr);
    if (FAILED(hr)) return false;

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = w;
    scDesc.Height = h;
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scDesc.Scaling = DXGI_SCALING_STRETCH;

    HR_CHECK_RET(factory->CreateSwapChainForHwnd(
        m_device.Get(), hwnd, &scDesc, nullptr, nullptr, &sc
    ));

    ComPtr<ID3D11Texture2D> backBuffer;
    HR_CHECK_RET(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));
    HR_CHECK_RET(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv));
    return true;
}

bool D3DRenderer::CreateOffscreenRT(UINT w, UINT h) {
    m_offscreenW = w;
    m_offscreenH = h;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = w;
    texDesc.Height = h;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HR_CHECK_RET(m_device->CreateTexture2D(&texDesc, nullptr, &m_offscreenTex));
    HR_CHECK_RET(m_device->CreateRenderTargetView(m_offscreenTex.Get(), nullptr, &m_offscreenRTV));
    HR_CHECK_RET(m_device->CreateShaderResourceView(m_offscreenTex.Get(), nullptr, &m_offscreenSRV));

    D3D11_TEXTURE2D_DESC dsDesc = texDesc;
    dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> dsTex;
    HR_CHECK_RET(m_device->CreateTexture2D(&dsDesc, nullptr, &dsTex));
    HR_CHECK_RET(m_device->CreateDepthStencilView(dsTex.Get(), nullptr, &m_dsv));

    return true;
}

const char* g_blitVSSrc = R"(
cbuffer BlitCB : register(b0) {
    float4x4 g_transform;
}
struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};
struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
PSInput main(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0), g_transform);
    output.uv = input.uv;
    return output;
}
)";

const char* g_blitPSSrc = R"(
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);
struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
float4 main(PSInput input) : SV_Target {
    return g_texture.Sample(g_sampler, input.uv);
}
)";

bool D3DRenderer::CompileBlitShaders() {
    ComPtr<ID3DBlob> vsBlob, psBlob;
    ComPtr<ID3DBlob> errors;

    HRESULT hr = D3DCompile(
        g_blitVSSrc, strlen(g_blitVSSrc),
        "blitVS.hlsl", nullptr, nullptr,
        "main", "vs_5_0", 0, 0,
        &vsBlob, &errors
    );

    if (FAILED(hr)) {
        if (errors) {
            LOG_ERROR("D3DRenderer: VS compile error: %s", (char*)errors->GetBufferPointer());
        }
        return false;
    }

    hr = D3DCompile(
        g_blitPSSrc, strlen(g_blitPSSrc),
        "blitPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0", 0, 0,
        &psBlob, &errors
    );

    if (FAILED(hr)) {
        if (errors) {
            LOG_ERROR("D3DRenderer: PS compile error: %s", (char*)errors->GetBufferPointer());
        }
        return false;
    }

    HR_CHECK_RET(m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_blitVS));
    HR_CHECK_RET(m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_blitPS));
    return true;
}

bool D3DRenderer::CreateBlitVB() {
    float vertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    HR_CHECK_RET(m_device->CreateBuffer(&vbDesc, &vbData, &m_blitVB));
    return true;
}

// 场景顶点着色器 inline 源码
const char* g_sceneVSSrc = R"(
cbuffer CB : register(b0) {
    float4x4 g_worldViewProj;
    float4   g_borderColor;
    float    g_borderWidth;
    float3   g_pad;
};

struct VS_IN {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = mul(float4(input.pos, 1.0f), g_worldViewProj);
    output.uv  = input.uv;
    return output;
}
)";

// 场景像素着色器 inline 源码
const char* g_scenePSSrc = R"(
Texture2D<float4> g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer CB : register(b0) {
    float4x4 g_worldViewProj;
    float4   g_borderColor;
    float    g_borderWidth;
    float3   g_pad;
};

struct PS_IN {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 PSMain(PS_IN input) : SV_TARGET {
    float2 uv = input.uv;
    float b = g_borderWidth;

    // 边框检测
    if (uv.x < b || uv.x > 1.0f - b || uv.y < b || uv.y > 1.0f - b)
        return g_borderColor;

    // 采样桌面纹理
    return g_texture.Sample(g_sampler, uv);
}
)";

bool D3DRenderer::LoadSceneShaders() {
    ComPtr<ID3DBlob> vsBlob, psBlob, errors;

    // 编译顶点着色器
    HRESULT hr = D3DCompile(
        g_sceneVSSrc, strlen(g_sceneVSSrc),
        "sceneVS.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0,
        &vsBlob, &errors
    );
    if (FAILED(hr)) {
        if (errors) LOG_ERROR("Scene VS compile error: %s", (char*)errors->GetBufferPointer());
        return false;
    }

    // 编译像素着色器
    hr = D3DCompile(
        g_scenePSSrc, strlen(g_scenePSSrc),
        "scenePS.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0,
        &psBlob, &errors
    );
    if (FAILED(hr)) {
        if (errors) LOG_ERROR("Scene PS compile error: %s", (char*)errors->GetBufferPointer());
        return false;
    }

    // 创建着色器对象
    HR_CHECK_RET(m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs));
    HR_CHECK_RET(m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps));

    // 创建场景采样器
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    HR_CHECK_RET(m_device->CreateSamplerState(&sampDesc, &m_sceneSampler));

    // 创建输入布局（匹配ScreenVertex结构）
    D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    HR_CHECK_RET(m_device->CreateInputLayout(
        inputLayout, ARRAYSIZE(inputLayout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        &m_inputLayout));

    LOG_INFO("D3DRenderer: Scene shaders compiled successfully");
    return true;
}

bool D3DRenderer::Init(HWND glassesHwnd, UINT width, UINT height) {
    LOG_INFO("D3DRenderer::Init: creating device");
    if (!CreateDevice()) return false;

    if (glassesHwnd) {
        LOG_INFO("D3DRenderer::Init: creating swapchain for glasses");
        if (!CreateSwapChain(glassesHwnd, width, height, m_glassesSwapChain, m_glassesRTV)) return false;
    } else {
        LOG_INFO("D3DRenderer::Init: no glasses hwnd, skipping swapchain");
    }

    LOG_INFO("D3DRenderer::Init: creating offscreen RT");
    if (!CreateOffscreenRT(width, height)) return false;
    LOG_INFO("D3DRenderer::Init: compiling shaders");
    if (!CompileBlitShaders()) return false;
    if (!LoadSceneShaders()) return false;
    LOG_INFO("D3DRenderer::Init: creating blit VB");
    if (!CreateBlitVB()) return false;

    m_sharedQuad = std::make_unique<ScreenQuad>();
    LOG_INFO("D3DRenderer::Init: creating screen quad");
    return m_sharedQuad->Init(m_device.Get(), 1.0f, 1.0f);
}

bool D3DRenderer::InitPreview(HWND previewHwnd, UINT w, UINT h) {
    if (!CreateSwapChain(previewHwnd, w, h, m_previewSwapChain, m_previewRTV)) return false;
    m_previewReady = true;
    LOG_INFO("D3DRenderer: Preview initialized %ux%u", w, h);
    return true;
}

bool D3DRenderer::InitScreenResources(int screenCount) {
    std::vector<std::pair<UINT, UINT>> sizes(screenCount, {m_offscreenW, m_offscreenH});
    return InitScreenResources(sizes);
}

bool D3DRenderer::InitScreenResources(const std::vector<std::pair<UINT, UINT>>& sizes) {
    int screenCount = static_cast<int>(sizes.size());
    m_textures.resize(screenCount);
    m_srvs.resize(screenCount);

    for (int i = 0; i < screenCount; ++i) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = sizes[i].first;
        desc.Height = sizes[i].second;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HR_CHECK_RET(m_device->CreateTexture2D(&desc, nullptr, &m_textures[i]));
        HR_CHECK_RET(m_device->CreateShaderResourceView(m_textures[i].Get(), nullptr, &m_srvs[i]));

        LOG_INFO("D3DRenderer: Screen texture[%d] = %ux%u", i, sizes[i].first, sizes[i].second);
    }

    LOG_INFO("D3DRenderer: Created %d screen textures", screenCount);
    return true;
}

bool D3DRenderer::InitCurvedScreen(int index, float width, float height, float curvature, int segments) {
    if (index >= (int)m_screenQuads.size()) {
        m_screenQuads.resize(index + 1);
    }

    m_screenQuads[index] = std::make_unique<ScreenQuad>();
    return m_screenQuads[index]->InitCurved(m_device.Get(), width, height, curvature, segments);
}

void D3DRenderer::Shutdown() {
    m_blitVS.Reset();
    m_blitPS.Reset();
    m_blitVB.Reset();
    m_constantBuffer.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_inputLayout.Reset();
    m_sceneSampler.Reset();
    m_previewRTV.Reset();
    m_previewSwapChain.Reset();
    m_offscreenRTV.Reset();
    m_offscreenSRV.Reset();
    m_offscreenTex.Reset();
    m_dsv.Reset();
    m_glassesRTV.Reset();
    m_glassesSwapChain.Reset();
    m_blitCB.Reset();
    m_blitSampler.Reset();
    m_context.Reset();
    m_device.Reset();
    m_srvs.clear();
    m_textures.clear();
    m_screenQuads.clear();
    m_sharedQuad.reset();
    m_previewReady = false;
    LOG_INFO("D3DRenderer: Shutdown complete");
}

void D3DRenderer::BeginFrame() {
    if (!m_offscreenRTV) return;
    m_context->OMSetRenderTargets(1, m_offscreenRTV.GetAddressOf(), m_dsv.Get());
    float clearColor[] = {m_bgColor[0], m_bgColor[1], m_bgColor[2], 1.0f};
    m_context->ClearRenderTargetView(m_offscreenRTV.Get(), clearColor);
    m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void D3DRenderer::EndFrame() {
    if (m_glassesSwapChain && m_glassesRTV) {
        BlitToTarget(m_glassesSwapChain.Get(), m_glassesRTV.Get());
        m_glassesSwapChain->Present(m_vsync ? 1 : 0, 0);
    }
}

void D3DRenderer::PresentPreview() {
    if (!m_previewReady || !m_previewSwapChain) return;
    BlitToTarget(m_previewSwapChain.Get(), m_previewRTV.Get());
    m_previewSwapChain->Present(m_vsync ? 1 : 0, 0);
}

void D3DRenderer::DrawScreen(const XMMATRIX& worldMatrix, const XMMATRIX& viewProj, ID3D11ShaderResourceView* textureSRV, int screenIndex) {
    if (!textureSRV || !m_vs || !m_ps) return;

    // 选择要使用的屏幕quad
    ScreenQuad* quad = nullptr;
    if (screenIndex >= 0 && screenIndex < (int)m_screenQuads.size() && m_screenQuads[screenIndex]) {
        quad = m_screenQuads[screenIndex].get();
    } else {
        quad = m_sharedQuad.get();
    }
    if (!quad) return;

    XMMATRIX wvp = viewProj * worldMatrix; // D3D矩阵乘法是右乘：顶点 * world * view * proj
    SceneCB cb;
    XMStoreFloat4x4(&cb.worldViewProj, XMMatrixTranspose(wvp)); // HLSL默认列主序，需要转置
    cb.borderColor = XMFLOAT4(m_borderColor[0], m_borderColor[1], m_borderColor[2], 1.0f);
    cb.borderWidth = m_borderWidth;
    cb.pad[0] = cb.pad[1] = cb.pad[2] = 0;

    if (!m_constantBuffer) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SceneCB);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        m_device->CreateBuffer(&desc, nullptr, &m_constantBuffer);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &cb, sizeof(cb));
    m_context->Unmap(m_constantBuffer.Get(), 0);

    // 绑定场景着色器和资源
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vs.Get(), nullptr, 0);
    m_context->PSSetShader(m_ps.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, &textureSRV);
    m_context->PSSetSamplers(0, 1, m_sceneSampler.GetAddressOf());

    // 绑定ScreenQuad的顶点和索引缓冲
    quad->Bind(m_context.Get());
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->DrawIndexed(quad->GetIndexCount(), 0, 0);
}

void D3DRenderer::UpdateTexture(size_t index, ID3D11Texture2D* sourceTex) {
    if (index >= m_textures.size()) return;

    m_context->CopyResource(m_textures[index].Get(), sourceTex);
}

ID3D11ShaderResourceView* D3DRenderer::GetTextureSRV(size_t index) const {
    if (index >= m_srvs.size()) return nullptr;
    return m_srvs[index].Get();
}

bool D3DRenderer::SaveScreenshot(const std::string& path) {
    if (!m_offscreenTex) return false;

    ComPtr<ID3D11Texture2D> stagingTex;
    D3D11_TEXTURE2D_DESC desc = {};
    m_offscreenTex->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    HR_CHECK_RET(m_device->CreateTexture2D(&desc, nullptr, &stagingTex));
    m_context->CopyResource(stagingTex.Get(), m_offscreenTex.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    HR_CHECK_RET(m_context->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped));

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(bih);
    bih.biWidth = desc.Width;
    bih.biHeight = -(LONG)desc.Height;
    bih.biBitCount = 32;
    bih.biPlanes = 1;
    bih.biCompression = BI_RGB;

    BITMAPFILEHEADER bfh = {};
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(bih);
    bfh.bfSize = bfh.bfOffBits + desc.Width * desc.Height * 4;

    FILE* file = nullptr;
    fopen_s(&file, path.c_str(), "wb");
    if (!file) {
        m_context->Unmap(stagingTex.Get(), 0);
        return false;
    }

    fwrite(&bfh, 1, sizeof(bfh), file);
    fwrite(&bih, 1, sizeof(bih), file);

    BYTE* pixels = (BYTE*)mapped.pData;
    for (UINT y = 0; y < desc.Height; ++y) {
        fwrite(pixels + y * mapped.RowPitch, 1, desc.Width * 4, file);
    }

    fclose(file);
    m_context->Unmap(stagingTex.Get(), 0);

    LOG_INFO("D3DRenderer: Screenshot saved: %s", path.c_str());
    return true;
}

void D3DRenderer::BlitToTarget(IDXGISwapChain1* sc, ID3D11RenderTargetView* rtv) {
    ComPtr<ID3D11Texture2D> backBuffer;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);

    ComPtr<ID3D11RenderTargetView> newRTV;
    m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &newRTV);

    m_context->OMSetRenderTargets(1, newRTV.GetAddressOf(), nullptr);

    float clearColor[] = {m_bgColor[0], m_bgColor[1], m_bgColor[2], 1.0f};
    m_context->ClearRenderTargetView(newRTV.Get(), clearColor);

    if (!m_blitCB) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(float) * 16;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        float identity[] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = identity;
        m_device->CreateBuffer(&desc, &data, &m_blitCB);
    }

    m_context->VSSetConstantBuffers(0, 1, m_blitCB.GetAddressOf());

    if (!m_blitSampler) {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        m_device->CreateSamplerState(&sampDesc, &m_blitSampler);
    }
    m_context->PSSetSamplers(0, 1, m_blitSampler.GetAddressOf());

    m_context->VSSetShader(m_blitVS.Get(), nullptr, 0);
    m_context->PSSetShader(m_blitPS.Get(), nullptr, 0);
    m_context->PSSetShaderResources(0, 1, m_offscreenSRV.GetAddressOf());

    UINT stride = sizeof(float) * 5;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_blitVB.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_context->Draw(4, 0);

    ID3D11RenderTargetView* nullRTV = nullptr;
    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
}