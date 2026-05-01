
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct ScreenVertex {
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

class ScreenQuad {
public:
    ScreenQuad() = default;

    bool Init(ID3D11Device* device, float width, float height);

    bool InitCurved(ID3D11Device* device, float width, float height,
                     float curvature, int segments);

    void Bind(ID3D11DeviceContext* ctx);
    UINT GetIndexCount() const { return m_indexCount; }

private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount = 0;
};
