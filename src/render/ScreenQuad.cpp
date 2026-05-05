#include "ScreenQuad.h"
#include "../utils/Log.h"
#include "../utils/ComHelper.h"
#include <cmath>

bool ScreenQuad::Init(ID3D11Device* device, float width, float height) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    ScreenVertex vertices[] = {
        { XMFLOAT3(-hw,  hh, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( hw,  hh, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( hw, -hh, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-hw, -hh, 0.0f), XMFLOAT2(0.0f, 1.0f) },
    };

    UINT indices[] = { 0, 1, 2, 0, 2, 3 };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    HR_CHECK_RET(device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer));

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;

    HR_CHECK_RET(device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer));
    m_indexCount = 6;

    LOG_INFO("ScreenQuad: Initialized %fx%f", width, height);
    return true;
}

bool ScreenQuad::InitCurved(ID3D11Device* device, float width, float height, float curvature, int segments) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float radius = curvature > 0.0f ? (1.0f / curvature) : 0.0f;

    std::vector<ScreenVertex> vertices;
    std::vector<UINT> indices;

    for (int row = 0; row <= segments; ++row) {
        float v = (float)row / segments;
        float y = (v - 0.5f) * height;

        for (int col = 0; col <= segments; ++col) {
            float u = (float)col / segments;
            float x = (u - 0.5f) * width;

            float z = 0.0f;
            if (radius > 0.0f) {
                float angle = (u - 0.5f) * curvature; // 左右对称分布的弧形，curvature为总弧度
                z = (float)(radius - radius * cos(angle));
            }

            vertices.push_back({
                XMFLOAT3(x, y, z),
                XMFLOAT2(u, v)
            });
        }
    }

    for (int row = 0; row < segments; ++row) {
        for (int col = 0; col < segments; ++col) {
            int i = row * (segments + 1) + col;
            indices.push_back(i);
            indices.push_back(i + segments + 1);
            indices.push_back(i + 1);
            indices.push_back(i + 1);
            indices.push_back(i + segments + 1);
            indices.push_back(i + segments + 2);
        }
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(sizeof(ScreenVertex) * vertices.size());
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    HR_CHECK_RET(device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer));

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(sizeof(UINT) * indices.size());
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    HR_CHECK_RET(device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer));
    m_indexCount = (UINT)indices.size();

    LOG_INFO("ScreenQuad: Curved initialized %fx%f, r=%f, seg=%d", width, height, curvature, segments);
    return true;
}

void ScreenQuad::Bind(ID3D11DeviceContext* ctx) {
    UINT stride = sizeof(ScreenVertex);
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}