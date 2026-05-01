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
