struct VertexInput
{
	float2 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer Camera : register(b0)
{
	float4x4 projectionMatrix;
}


VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.position = mul(float4(input.position, 1, 1), projectionMatrix);
	output.uv = float2(input.uv);
	return output;
}