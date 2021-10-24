struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 uv : TEXCOORD;
};

cbuffer Application : register(b0)
{

}

cbuffer Camera : register(b1)
{
    float4x4 viewProjMatrix;
}

cbuffer ObjectConstants : register(b2)
{
    float4x4 objMatrix;
}

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    output.position = mul(float4(input.position, 1), objMatrix);
    output.position = mul(output.position, viewProjMatrix);
    output.normal = float4(input.normal, 1);
    output.uv = float4(input.uv, 0, 0);

    return output;
}