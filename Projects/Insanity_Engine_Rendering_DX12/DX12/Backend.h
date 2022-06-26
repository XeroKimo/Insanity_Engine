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

    class Backend
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    public:
        gsl::not_null<TypedD3D::Wrapper<ID3D12Device5>> device;
        gsl::not_null<TypedD3D::Direct<ID3D12CommandQueue>> commandQueue;
        gsl::not_null<ComPtr<ID3D12Fence>> fence;

    private:
        gsl::not_null<TypedD3D::Wrapper<IDXGISwapChain3>> m_swapChain;
        std::vector<TypedD3D::Wrapper<ID3D12Resource>> m_frameResources;
        gsl::not_null<TypedD3D::RTV<ID3D12DescriptorHeap>> m_swapChainDescriptorHeap;
        std::vector<UINT64> m_frameFenceValues;
        UINT64 m_rtvHandleIncrement;
        UINT64 m_highestFrameFenceValue = 0;
        UINT m_backBufferIndex = 0;

    public:
        Backend(HWND windowHandle,
            Math::Types::Vector2ui size, 
            gsl::not_null<TypedD3D::Wrapper<ID3D12Device5>> device,
            gsl::not_null<TypedD3D::Wrapper<IDXGIFactory2>> factory,
            UINT bufferCount = 2,
            DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
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
        gsl::not_null<TypedD3D::Wrapper<ID3D12Resource>> GetCurrentFrameResource() const { return m_frameResources[GetCurrentFrameIndex()]; }
        gsl::not_null<TypedD3D::Wrapper<ID3D12Resource>> GetPreviousFrameResource() const { return m_frameResources[GetPreviousFrameIndex()]; }

        UINT64& GetCurrentFrameFenceValue() { return m_frameFenceValues[GetCurrentFrameIndex()]; }
        UINT64 GetPreviousFrameFenceValue() const { return GetFrameFenceValue(GetPreviousFrameIndex()); }

        const UINT64& GetCurrentFrameFenceValue() const { return m_frameFenceValues[GetCurrentFrameIndex()]; }

        UINT64 GetFrameFenceValue(size_t frame) const { return m_frameFenceValues[frame]; }
        DXGI_SWAP_CHAIN_DESC1 GetSwapChainDescription() const { return m_swapChain->GetDesc1(); }
        UINT GetBufferCount() const { return static_cast<UINT>(m_frameResources.size()); }

        UINT GetCurrentFrameIndex() const { return m_backBufferIndex; }
        UINT GetPreviousFrameIndex() const { return (std::min)(m_backBufferIndex - 1, static_cast<UINT>(m_frameResources.size() - 1)); }

        virtual void Flush()
        {
            TypedD3D::Helpers::D3D12::FlushCommandQueue(*commandQueue.get().Get(), *fence.get().Get(), GetCurrentFrameFenceValue());
        }

        TypedD3D::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> GetCurrentFrameHandle()
        {
            return m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Offset(GetCurrentFrameIndex(), m_rtvHandleIncrement);
        }

    public:
        template<std::invocable<const FrameData&> Func>
        void Present(UINT syncInterval, UINT flags, Func&& func)
        {
            TypedD3D::Helpers::D3D12::PrependCPUWait({ *fence.get().Get() }, GetCurrentFrameFenceValue(), [&]()
            {
                GetCurrentFrameFenceValue() = m_highestFrameFenceValue;

                func(FrameData
                {
                    .commandQueue = commandQueue,
                    .fence = fence,
                    .currentFrameResource = m_frameResources[GetCurrentFrameIndex()],
                    .currentFrameHandle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Offset(GetCurrentFrameIndex(), m_rtvHandleIncrement),
                    .currentFrameFenceValue = GetCurrentFrameFenceValue(),
                    .currentFrameIndex = GetCurrentFrameIndex(),
                });

                GetCurrentFrameFenceValue() = m_highestFrameFenceValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*commandQueue.get().Get(), *fence.get().Get(), GetCurrentFrameFenceValue());
                m_swapChain->Present(1, 0);
                m_backBufferIndex = (m_backBufferIndex + 1) % static_cast<UINT>(m_frameFenceValues.size());
            });
        }

    private:
        void Reset();

    private:
        void CreateRenderTargets();
    };



    template<class Renderer>
    class BackendWithRenderer : public Backend
    {
        Renderer m_renderer;

    public:
        template<class... Args>
        BackendWithRenderer(HWND windowHandle,
            Math::Types::Vector2ui size,
            gsl::not_null<TypedD3D::Wrapper<ID3D12Device5>> device,
            gsl::not_null<TypedD3D::Wrapper<IDXGIFactory2>> factory,
            UINT bufferCount = 2,
            DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
            Args&&... args) :
            Backend(windowHandle,
                size,
                device,
                factory,
                bufferCount,
                swapChainFormat),
            m_renderer(static_cast<Backend&>(*this), std::forward<Args>(args)...)
        {
        }

    private:
        using Backend::Present;

    public:
        void Draw()
        {
            Present(1, 0, [&](const FrameData& frameData)
            {
                m_renderer.Draw(frameData);
            });
        }

        void Flush() final
        {
            m_renderer.Flush();
            Backend::Flush();
        }

        Renderer& GetRenderer() { return m_renderer; }
        const Renderer& GetRenderer() const { return m_renderer; }
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
                Flush(static_cast<UINT>(i));
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