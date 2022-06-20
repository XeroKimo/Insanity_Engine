#pragma once
#include "Insanity_Math.h"
#include <d3d12.h>

namespace InsanityEngine::Rendering::D3D12
{
    struct MeshObject
    {
        Math::Types::Vector3f position;
        Math::Types::Vector3f scale;
        Math::Types::Quaternion<float> quaternion;

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_GPU_DESCRIPTOR_HANDLE constantBufferView;
    };
}