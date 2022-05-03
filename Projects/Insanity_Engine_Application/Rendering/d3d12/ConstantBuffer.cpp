#include "ConstantBuffer.h"
#include <d3dx12.h>

namespace InsanityEngine::Rendering::D3D12
{
    ConstantBuffer::ConstantBuffer(TypedD3D::D3D12::Device device, UINT64 size, UINT64 startingFenceValue) :
        m_resource(device->CreateCommittedResource(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
            CD3DX12_RESOURCE_DESC::Buffer(Utility::AlignCeiling(size, static_cast<UINT64>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr).GetValue()),
        m_fenceValue(startingFenceValue)
    {
        void* mappedPtr;
        m_resource->Map(0, nullptr, &mappedPtr);
        m_begin = m_current = static_cast<char*>(mappedPtr);
        m_end = m_begin + m_resource->GetDesc().Width + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        m_entries.push_back(
            {
                .begin = m_current,
                .end = m_current,
                .fenceValue = m_fenceValue
            });
    }

    ConstantBuffer::~ConstantBuffer()
    {
        m_resource->Unmap(0, nullptr);
    }

    D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::push_back(const void* data, UINT64 size)
    {
        if(m_current + size >= m_end)
        {
            m_current = m_begin;
        }

        if(m_current < m_entries.front().begin &&
            m_current + size >= m_entries.front().begin)
        {
            throw std::bad_alloc();
        }

        D3D12_GPU_VIRTUAL_ADDRESS address = m_resource->GetGPUVirtualAddress() + (m_current - m_begin);
        std::memcpy(m_current, data, size);
        m_entries.back().end = m_current = Utility::AlignCeiling(m_current + size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        return address;
    }

    void ConstantBuffer::Signal(UINT64 fenceValue)
    {
        m_fenceValue = m_entries.back().fenceValue = fenceValue;

        m_entries.push_back(Entry
            {
                .begin = m_current,
                .end = m_current,
                .fenceValue = m_fenceValue + 1
            });
    }

    void ConstantBuffer::Clear(UINT64 fenceValue)
    {
        while(!m_entries.empty() && m_entries.front().fenceValue <= fenceValue)
        {
            m_entries.pop_front();
        }

        if(m_entries.empty())
        {
            m_current = m_begin;
            m_entries.push_back(
                {
                    .begin = m_current,
                    .end = m_current,
                    .fenceValue = m_fenceValue + 1
                });
        }
    }
}
