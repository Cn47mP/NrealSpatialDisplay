
#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include "ScreenQuad.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct SceneCB {
    XMFLOAT4X4 worldViewProj;
    XMFLOAT4   borderColor;
    float      borderWidth;
    float      pad[3];
};

class D3DRenderer {
public:
    D3DRenderer() = default;
    ~D3DRenderer() { Shutdown(); }

    bool Init(HWND glassesHwnd, UINT width, UINT height);
    bool InitPreview(HWND previewHwnd, UINT w, UINT h);
    bool InitScreenResources(int screenCount);
    bool InitCurvedScreen(int index, float width, float height,
                          float curvature, int segments);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void PresentPreview();

    void DrawScreen(const XMMATRIX& worldMatrix, const XMMATRIX& viewProj,
                    ID3D11ShaderResourceView* textureSRV, int screenIndex = -1);

    void UpdateTexture(size_t index, ID3D11Texture2D* sourceTex);
    ID3D11ShaderResourceView* GetTextureSRV(size_t index) const;

    bool SaveScreenshot(const std::string& path);

    void SetBackgroundColor(float r, float g, float b) { m_bgColor[0] = r; m_bgColor[1] = g; m_bgColor[2] = b; }
    void SetBorderColor(float r, float g, float b) { m_borderColor[0] = r; m_borderColor[1] = g; m_borderColor[2] = b; }
    void SetBorderWidth(float w) { m_borderWidth = w; }
    void SetVSync(bool enabled) { m_vsync = enabled; }
    bool IsPreviewReady() const { return m_previewReady; }

    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;

    ComPtr<IDXGISwapChain1> m_glassesSwapChain;
    ComPtr<ID3D11RenderTargetView> m_glassesRTV;

    ComPtr<IDXGISwapChain1> m_previewSwapChain;
    ComPtr<ID3D11RenderTargetView> m_previewRTV;
    bool m_previewReady = false;

    ComPtr<ID3D11Texture2D> m_offscreenTex;
    ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
    ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;
    ComPtr<ID3D11DepthStencilView> m_dsv;

    ComPtr<ID3D11VertexShader> m_blitVS;
    ComPtr<ID3D11PixelShader> m_blitPS;
    ComPtr<ID3D11Buffer> m_blitVB;
    ComPtr<ID3D11Buffer> m_blitCB;
    ComPtr<ID3D11SamplerState> m_blitSampler;

    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11Buffer> m_constantBuffer;

    std::vector<ComPtr<ID3D11Texture2D>> m_textures;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_srvs;

    std::unique_ptr<ScreenQuad> m_sharedQuad;
    std::vector<std::unique_ptr<ScreenQuad>> m_screenQuads;

    float m_bgColor[3] = {0.02f, 0.02f, 0.04f};
    float m_borderColor[3] = {0.3f, 0.3f, 0.3f};
    float m_borderWidth = 0.005f;
    bool m_vsync = true;
    UINT m_offscreenW = 0;
    UINT m_offscreenH = 0;

    bool CreateDevice();
    bool CreateSwapChain(HWND hwnd, UINT w, UINT h, ComPtr<IDXGISwapChain1>& sc,
                         ComPtr<ID3D11RenderTargetView>& rtv);
    bool CreateOffscreenRT(UINT w, UINT h);
    bool CompileBlitShaders();
    bool LoadSceneShaders();
    bool CreateBlitVB();

    void BlitToTarget(IDXGISwapChain1* sc, ID3D11RenderTargetView* rtv);
};
