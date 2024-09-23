struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D main_texture : register(t0);

SamplerState smp
{
    Filter = MIN_MAG_MIP_POINT;
};

float4 main(VS_OUT input) : SV_TARGET
{
    return main_texture.Sample(smp, input.uv);
}