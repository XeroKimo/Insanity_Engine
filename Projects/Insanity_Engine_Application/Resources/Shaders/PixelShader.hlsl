

struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 uv : TEXCOORD;
};

Texture2D albedoTexture : register(t0);
SamplerState albedoSampler : register(s0);

cbuffer ApplicationConstants : register(b0)
{

}

cbuffer MaterialConstants : register(b2)
{
	float4 color;
}

float4 main(VertexOutput input) : SV_TARGET
{
	return albedoTexture.Sample(albedoSampler, input.uv.xy * 2) * color;
}