

struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState sam : register(s0);

cbuffer ApplicationConstants : register(b0)
{

}

cbuffer ObjectConstants : register(b1)
{
	float4 color;
}

float4 main(VertexOutput input) : SV_TARGET
{
	return tex.Sample(sam, input.uv.xy * 2);
}