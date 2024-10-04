struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D main_texture : register(t0);

SamplerState point_sampler : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
    return main_texture.Sample(point_sampler, input.uv);
}