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

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.position = float4(input.position, 0, 1);
	output.uv = float2(input.uv);
	return output;
}