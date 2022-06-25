#pragma once
#include "TypedD3D12.h"
#include "TypedDXGI.h"
#include "Insanity_Math.h"
#include "MeshObject.h"
#include <wrl/client.h>
#include <vector>
#include <gsl/gsl>

namespace InsanityEngine::Rendering::D3D12
{
    struct FrameData
    {
        gsl::not_null<TypedD3D::Direct<ID3D12CommandQueue>> commandQueue;
        gsl::not_null<Microsoft::WRL::ComPtr<ID3D12Fence>> fence;
        gsl::not_null<TypedD3D::Wrapper<ID3D12Resource>> currentFrameResource;
        TypedD3D::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> currentFrameHandle;
        UINT64& currentFrameFenceValue;
        UINT64 currentFrameIndex;
    };

    struct BackendInitParams
    {
        TypedD3D::Wrapper<ID3D12Device5> device;
        TypedD3D::Wrapper<IDXGIFactory2> factory;
        HWND windowHandle;
        Math::Types::Vector2ui windowSize;
        UINT bufferCount = 2;
        DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    };

    class Backend
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    protected:
        gsl::not_null<TypedD3D::Wrapper<ID3D12Device5>> m_device;
        gsl::not_null<TypedD3D::Direct<ID3D12CommandQueue>> m_mainQueue;
        gsl::not_null<TypedD3D::Wrapper<IDXGISwapChain3>> m_swapChain;
        std::vector<TypedD3D::Wrapper<ID3D12Resource>> m_frameResources;
        gsl::not_null<TypedD3D::RTV<ID3D12DescriptorHeap>> m_swapChainDescriptorHeap;
        gsl::not_null<ComPtr<ID3D12Fence>> m_mainFence;
        std::vector<UINT64> m_frameFenceValues;
        UINT64 m_rtvHandleIncrement;
        UINT64 m_highestFrameFenceValue = 0;
        UINT m_backBufferIndex = 0;

    public:
        Backend(const BackendInitParams& params);
        ~Backend();

    public:
        void ResizeBuffers(Math::Types::Vector2ui size);
        void SetFullscreen(bool fullscreen);
        void SetWindowSize(Math::Types::Vector2ui size);

    public:
        bool IsFullscreen() const;
        Math::Types::Vector2ui GetWindowSize() const
        {
            auto description = m_swapChain->GetDesc1();
            return { description.Width, description.Height };
        }

    public:
        gsl::not_null<TypedD3D::Direct<ID3D12CommandQueue>> GetCommandQueue() const { return m_mainQueue; }
        gsl::not_null<ComPtr<ID3D12Fence>> GetFence() const { return m_mainFence; }
        gsl::not_null<TypedD3D::Wrapper<ID3D12Resource>> GetCurrentFrameResource() const { return m_frameResources[GetCurrentFrameIndex()]; }
        gsl::not_null<TypedD3D::Wrapper<ID3D12Resource>> GetPreviousFrameResource() const { return m_frameResources[GetPreviousFrameIndex()]; }

        UINT64& GetCurrentFenceValue() { return GetFrameFenceValue(GetCurrentFrameIndex()); }
        UINT64& GetPreviousFenceValue() { return GetFrameFenceValue(GetPreviousFrameIndex()); }

        const UINT64& GetCurrentFenceValue() const { return GetFrameFenceValue(GetCurrentFrameIndex()); }
        const UINT64& GetPreviousFenceValue() const { return GetFrameFenceValue(GetPreviousFrameIndex()); }

        UINT64& GetFrameFenceValue(size_t frame) { return m_frameFenceValues[frame]; }
        const UINT64& GetFrameFenceValue(size_t frame) const { return m_frameFenceValues[frame]; }
        DXGI_SWAP_CHAIN_DESC1 GetSwapChainDescription() const { return m_swapChain->GetDesc1(); }
        UINT GetBufferCount() const { return static_cast<UINT>(m_frameResources.size()); }

        UINT GetCurrentFrameIndex() const { return m_backBufferIndex; }
        UINT GetPreviousFrameIndex() const { return (std::min)(m_backBufferIndex - 1, static_cast<UINT>(m_frameResources.size() - 1)); }

        virtual void Flush()
        {
            TypedD3D::Helpers::D3D12::FlushCommandQueue(*m_mainQueue.get().Get(), *m_mainFence.get().Get(), m_frameFenceValues[m_backBufferIndex]);
        }

        TypedD3D::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> GetCurrentFrameHandle()
        {
            return m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Offset(GetCurrentFrameIndex(), m_rtvHandleIncrement);
        }
    private:
        void Reset();

    public:
        TypedD3D::D3D12::Device5 GetDevice() const { return m_device; }

    private:
        void CreateRenderTargets();
    };



    template<class Renderer>
    class BackendWithRenderer : public Backend
    {
        Renderer m_renderer;

    public:
        template<class... Args>
        BackendWithRenderer(const BackendInitParams& params, Args&&... args) :
            Backend(params),
            m_renderer(static_cast<Backend&>(*this), std::forward<Args>(args)...)
        {
        }

    public:
        void Draw()
        {
            DrawImpl();
        }

        void Flush() final
        {
            m_renderer.Flush();
            Backend::Flush();
        }

        Renderer& GetRenderer() { return m_renderer; }
        const Renderer& GetRenderer() const { return m_renderer; }

    private:
        void DrawImpl()
        {
            TypedD3D::Helpers::D3D12::CPUWaitAndThen({ *m_mainFence.get().Get() }, m_frameFenceValues[m_backBufferIndex], [&]()
            {
                m_frameFenceValues[m_backBufferIndex] = m_highestFrameFenceValue;
                FrameData f =
                {
                    .commandQueue = m_mainQueue,
                    .fence = m_mainFence,
                    .currentFrameResource = m_frameResources[m_backBufferIndex],
                    .currentFrameHandle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Offset(m_backBufferIndex, m_rtvHandleIncrement),
                    .currentFrameFenceValue = m_frameFenceValues[m_backBufferIndex],
                    .currentFrameIndex = m_backBufferIndex,
                };
                m_renderer.Draw(f);

                m_frameFenceValues[m_backBufferIndex] = m_highestFrameFenceValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*m_mainQueue.get().Get(), *m_mainFence.get().Get(), m_frameFenceValues[m_backBufferIndex]);
                m_swapChain->Present(1, 0);
                m_backBufferIndex = (m_backBufferIndex + 1) % static_cast<UINT>(m_frameFenceValues.size());
            });
        }
    };


    class DefaultAllocatorManager
    {
        struct Allocators
        {
            std::vector<TypedD3D::Direct<ID3D12CommandAllocator>> allocators;
            size_t idleAllocator;
        };

        TypedD3D::Wrapper<ID3D12Device5> m_device;
        std::vector<Allocators> m_allocators;

    public:
        DefaultAllocatorManager(TypedD3D::Wrapper<ID3D12Device5> device, UINT bufferCount) :
            m_device(device),
            m_allocators(bufferCount)
        {

        }

    public:
        TypedD3D::Direct<ID3D12CommandAllocator> CreateOrGetAllocator(UINT currentFrameIndex)
        {
            Allocators& currentFrame = m_allocators[currentFrameIndex];
            TypedD3D::D3D12::CommandAllocator::Direct allocator;
            if(currentFrame.idleAllocator == currentFrame.allocators.size())
            {
                currentFrame.allocators.push_back(m_device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>().value());
                allocator = currentFrame.allocators.back();
            }
            else
            {
                allocator = currentFrame.allocators[currentFrame.idleAllocator];
                allocator->Reset();
            }
            ++currentFrame.idleAllocator;
            return allocator;
        }

        void Flush(UINT currentFrameIndex)
        {
            m_allocators[currentFrameIndex].idleAllocator = 0;
        }

        void Flush()
        {
            for(size_t i = 0; i < m_allocators.size(); i++)
            {
                Flush(i);
            }
        }
    };

    struct DefaultDraw
    {

        gsl::strict_not_null<Backend*> m_renderer;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;
        DefaultAllocatorManager m_allocatorManager;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        MeshObject m_mesh;

    public:
        DefaultDraw(Backend& renderer);

    public:
        void Draw(const FrameData& frameData);
        void Flush()
        {
            m_allocatorManager.Flush();
        }
    };
}