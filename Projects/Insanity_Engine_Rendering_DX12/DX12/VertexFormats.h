#pragma once
#include "Common/VertexFormats.h"
#include <d3d12.h>
#include <array>
#include <wrl/client.h>

namespace InsanityEngine::Rendering::D3D12::VertexFormat
{
    using namespace Common::VertexFormat;

    namespace Position
    {
        using Format = Common::VertexFormat::Position::Format;
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

        static_assert(sizeof(Format) == sizeof(float) * 3);

        inline D3D12_INPUT_LAYOUT_DESC layout
        {
            .pInputElementDescs = elements.data(),
            .NumElements = static_cast<UINT>(elements.size())
        };
    }

    namespace PositionNormalUV
    {
        using Format = Common::VertexFormat::PositionNormalUV::Format;
        inline constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 3> elements
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
                .SemanticName = "NORMAL",
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

        static_assert(sizeof(Format) == sizeof(float) * 8);

        inline D3D12_INPUT_LAYOUT_DESC layout
        {
            .pInputElementDescs = elements.data(),
            .NumElements = static_cast<UINT>(elements.size())
        };
    }
}