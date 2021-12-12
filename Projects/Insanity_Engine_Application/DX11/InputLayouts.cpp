#include "InputLayouts.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include <d3dcompiler.h>

namespace InsanityEngine::DX11::InputLayouts
{
    namespace PositionNormalUV
    {
        ComPtr<ID3D11InputLayout> InsanityEngine::DX11::InputLayouts::PositionNormalUV::CreateInputLayout(ID3D11Device* device)
        {
            constexpr std::string_view shaderProgram
            {
                "                   \
                struct VertexInput \
                {       \
                    float3 position : POSITION;    \
                    float3 normal : NORMAL;            \
                    float2 uv : TEXCOORD;\
                };                           \
                                    \
                float4 main(VertexInput input) : SV_POSITION \
                {\
                        return float4(0, 0, 0, 1);\
                }\
                "
            };

            ComPtr<ID3DBlob> blob;
            HRESULT hr = D3DCompile(shaderProgram.data(), shaderProgram.size(), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &blob, nullptr);

            if(FAILED(hr))
                throw Debug::Exceptions::HRESULTException("Failed to compile shader", hr);

            ComPtr<ID3D11InputLayout> inputLayout;
            hr = device->CreateInputLayout(elementDescription.data(), static_cast<UINT>(elementDescription.size()), blob->GetBufferPointer(), blob->GetBufferSize(), &inputLayout);

            if(FAILED(hr))
                throw Debug::Exceptions::HRESULTException("Failed to create input layout", hr);

            return inputLayout;
        }
    }
}