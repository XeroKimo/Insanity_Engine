struct VS_IN
{
    float3 position : POSITION;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
};

cbuffer PerCamera : register(b1)
{
    float4x4 cameraTransform;
}

VS_OUT main(VS_IN input) : SV_POSITION
{
    VS_OUT output;
    output.position = mul(cameraTransform, float4(input.position, 1));
    return output;
}