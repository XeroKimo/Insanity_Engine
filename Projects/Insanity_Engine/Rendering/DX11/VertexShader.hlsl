struct VS_IN
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_IN_PER_INSTANCE
{
    float4x4 objTransform : OBJTRANSFORM;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer PerFrame : register(b0)
{
    
}

cbuffer PerCamera : register(b1)
{
    float4x4 cameraTransform;
}

cbuffer PerMaterial : register(b2)
{
    
}

cbuffer PerObject : register(b3)
{
    
}

VS_OUT main(VS_IN input, VS_IN_PER_INSTANCE perInstance)
{
    VS_OUT output;
    output.position = mul(perInstance.objTransform, float4(input.position, 1));
    output.position = mul(cameraTransform, output.position);
    output.uv = input.uv;
	return output;
}