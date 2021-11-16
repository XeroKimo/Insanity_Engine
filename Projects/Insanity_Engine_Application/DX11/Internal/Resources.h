#pragma once
#include "../CommonInclude.h"

namespace InsanityEngine
{
    namespace DX11
    {
        struct Mesh
        {
            ComPtr<ID3D11Buffer> vertexBuffer;
            ComPtr<ID3D11Buffer> indexBuffer;

            UINT vertexCount = 0;
            UINT indexCount = 0;
        };

        struct Texture
        {
            ComPtr<ID3D11ShaderResourceView> shaderResource;
            ComPtr<ID3D11SamplerState> sampler;
        };

        struct Shader
        {
            ComPtr<ID3D11VertexShader> vertexShader;
            ComPtr<ID3D11PixelShader> pixelShader;
        };
    }
}