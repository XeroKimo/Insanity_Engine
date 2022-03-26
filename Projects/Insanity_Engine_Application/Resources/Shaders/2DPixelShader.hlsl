

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};


Texture2D albedoTexture : register(t0);
SamplerState albedoSampler : register(s0);

float4 main(VertexOutput input) : SV_TARGET
{
	return albedoTexture.Sample(albedoSampler, input.uv.xy);
}