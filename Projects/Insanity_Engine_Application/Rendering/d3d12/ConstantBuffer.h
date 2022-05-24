#pragma once
#include "TypedD3D12.h"
#include "../../Utility/Align.h"
#include <d3d12.h>
#include <deque>
#include <concepts>
#include <bit>

namespace InsanityEngine::Rendering::D3D12
{
    class ConstantBuffer
    {
        template<class Ty>
        using ComPtr = Microsoft::WRL::ComPtr<Ty>;
    private:
        struct Entry
        {
            char* begin;
            char* end;
            UINT64 fenceValue;
        };

    private:
        ComPtr<ID3D12Resource> m_resource;
        std::deque<Entry> m_entries;
        char* m_begin;
        char* m_current;
        char* m_end;
        UINT64 m_fenceValue;

    public:
        ConstantBuffer(TypedD3D::D3D12::Device device, UINT64 size, UINT64 startingFenceValue);
        ConstantBuffer(const ConstantBuffer& other) = delete;
        ConstantBuffer(ConstantBuffer&& other) noexcept = default;
        ~ConstantBuffer();

        ConstantBuffer& operator=(const ConstantBuffer& other) = delete;
        ConstantBuffer& operator=(ConstantBuffer&& other) noexcept = default;

    public:
        D3D12_GPU_VIRTUAL_ADDRESS push_back(const void* data, UINT64 size);

        template<class Ty>
            requires std::is_standard_layout_v<Ty>
        D3D12_GPU_VIRTUAL_ADDRESS emplace_back(const Ty& data)
        {
            return push_back(&data, sizeof(Ty));
        }

    public:
        size_t Size() const
        {
            if(m_entries.back().end > m_entries.front().begin)
            {
                return m_entries.back().end - m_entries.front().begin;
            }
            else
            {
                return m_end - m_entries.front().begin + m_entries.back().end - m_begin;
            }
        }
        size_t Capacity() const { return m_end - m_begin; }
        size_t AvailableSize() const { return Capacity() - Size(); }

    public:
        void Signal(UINT64 fenceValue);
        void Clear(UINT64 fenceValue);
        UINT64 GetFenceValue() const { return m_fenceValue; }

    public:
        ComPtr<ID3D12Resource> Data() const { return m_resource; }
    };
}