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
