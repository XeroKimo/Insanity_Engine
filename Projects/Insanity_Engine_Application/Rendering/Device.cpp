#include "Device.h"

namespace InsanityEngine::Rendering
{
    Device::DX12::DX12() :
        m_device(TypedD3D::D3D12::CreateDevice<TypedD3D::D3D12::Device5>(D3D_FEATURE_LEVEL_12_0, nullptr).value())
    {
    }
}
