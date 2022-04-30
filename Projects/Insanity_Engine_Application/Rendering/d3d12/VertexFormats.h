#pragma once
#include "../Common/VertexFormats.h"
#include <d3d12.h>
#include <array>
#include <wrl/client.h>

namespace InsanityEngine::Rendering::D3D12::VertexFormat
{
    using namespace Common::VertexFormat;

    namespace Position
    {
        inline constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 1> elements
        {
            D3D12_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            }
        };

        static_assert(sizeof(Common::VertexFormat::Position) == sizeof(float) * 3);

        inline D3D12_INPUT_LAYOUT_DESC layout
        {
            .pInputElementDescs = elements.data(),
            .NumElements = static_cast<UINT>(elements.size())
        };
    }

    namespace PositionUV
    {
        inline constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> elements
        {
            D3D12_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
            D3D12_INPUT_ELEMENT_DESC
            {
                .SemanticName = "TEXCOORD",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            },
        };

        static_assert(sizeof(Common::VertexFormat::PositionUV) == sizeof(float) * 5);

        inline D3D12_INPUT_LAYOUT_DESC layout
        {
            .pInputElementDescs = elements.data(),
            .NumElements = static_cast<UINT>(elements.size())
        };
    }
}