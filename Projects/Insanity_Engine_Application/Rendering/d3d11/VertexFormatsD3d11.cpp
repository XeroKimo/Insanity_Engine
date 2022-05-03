#include "VertexFormats.h"

namespace InsanityEngine::Rendering::D3D11::VertexFormat
{
    using Microsoft::WRL::ComPtr;

    namespace Position
    {
        ComPtr<ID3D11InputLayout> CreateLayout(ID3D11Device& device, ID3DBlob& shaderBlob)
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            device.CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), shaderBlob.GetBufferPointer(), shaderBlob.GetBufferSize(), &inputLayout);
            return inputLayout;
        }
    }

    namespace PositionNormalUV
    {
        ComPtr<ID3D11InputLayout> CreateLayout(ID3D11Device& device, ID3DBlob& shaderBlob)
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            device.CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), shaderBlob.GetBufferPointer(), shaderBlob.GetBufferSize(), &inputLayout);
            return inputLayout;
        }
    }
}