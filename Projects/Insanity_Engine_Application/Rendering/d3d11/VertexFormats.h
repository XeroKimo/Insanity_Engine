#pragma once
#include "../Common/VertexFormats.h"
#include <d3d11.h>
#include <array>
#include <wrl/client.h>

namespace InsanityEngine::Rendering::D3D11::VertexFormat
{
    using namespace Common::VertexFormat;

    namespace Position
    {
        inline constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements
        {
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            }
        };

        static_assert(sizeof(Common::VertexFormat::Position) == sizeof(float) * 3);

        Microsoft::WRL::ComPtr<ID3D11InputLayout> CreateLayout(ID3D11Device& device, ID3DBlob& shaderBlob);
    };

    namespace PositionUV
    {
        inline constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements
        {
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "TEXCOORD",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
        };

        static_assert(sizeof(Common::VertexFormat::PositionUV) == sizeof(float) * 5);

        Microsoft::WRL::ComPtr<ID3D11InputLayout> CreateLayout(ID3D11Device& device, ID3DBlob& shaderBlob);
    }

}