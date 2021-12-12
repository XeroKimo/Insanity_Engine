#pragma once
#include "CommonInclude.h"
#include <array>
#include "Insanity_Math.h"

namespace InsanityEngine::DX11::InputLayouts
{
    namespace PositionNormalUV
    {
        struct VertexData
        {
            Math::Types::Vector3f position;
            Math::Types::Vector3f normal;
            Math::Types::Vector2f uv;
        };

        inline constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 3> elementDescription
        {
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = 0,
                .InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "NORMAL",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "TEXCOORD",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            }
        };
        extern ComPtr<ID3D11InputLayout> CreateInputLayout(ID3D11Device* device);
    }

}