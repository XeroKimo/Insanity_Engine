#pragma once
#include "../WindowBackend.h"
#include "TypedD3D.h"
#include "Insanity_Math.h"
#include "MeshObject.h"
#include <wrl/client.h>
#include <vector>
#include <gsl/gsl>

namespace InsanityEngine::Rendering
{
    class Window;
}

namespace InsanityEngine::Rendering::D3D12
{
    class Backend : public BackendInterface
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    private:
        struct FrameData
        {
            size_t idleAllocatorIndex = 0;
            std::vector<TypedD3D::D3D12::CommandAllocator::Direct> allocators;
            UINT64 fenceWaitValue;
        };

    private:
        gsl::strict_not_null<Window*> m_window;
        TypedD3D::D3D12::Device5 m_device;
        TypedD3D::D3D12::CommandQueue::Direct m_mainQueue;
        ComPtr<IDXGISwapChain4> m_swapChain;
        TypedD3D::D3D12::DescriptorHeap::RTV m_swapChainDescriptorHeap;
        ComPtr<ID3D12Fence> m_mainFence;
        UINT64 m_previousFrameFenceValue = 0;
        std::vector<FrameData> m_frameData;

    public:
        Backend(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device);
        ~Backend();

    private:
        void ResizeBuffers(Math::Types::Vector2ui size) final;
        void SetFullscreen(bool fullscreen) final;
        void SetWindowSize(Math::Types::Vector2ui size) final;

    public:
        bool IsFullscreen() const final;
        Math::Types::Vector2ui GetWindowSize() const final
        {
            auto description = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
            return { description.Width, description.Height };
        }

    public:
        void SignalQueue();
        void WaitForCurrentFrame();
        void Present();

        template<size_t Extents>
        void ExecuteCommandLists(std::span<TypedD3D::D3D12::CommandList::Direct, Extents> commandLists)
        {
            m_mainQueue->ExecuteCommandLists(commandLists);
        }

        TypedD3D::D3D12::CommandAllocator::Direct CreateOrGetAllocator();
        TypedD3D::D3D12::DescriptorHandle::CPU_RTV GetBackBufferHandle();
        Microsoft::WRL::ComPtr<ID3D12Resource> GetBackBufferResource();

    public:
        UINT GetCurrentBackBufferIndex() const;
        UINT64 GetCurrentFenceValue() const { return GetFrameFenceValue(m_swapChain->GetCurrentBackBufferIndex()); }
        UINT64 GetFrameFenceValue(size_t frame) const { return m_frameData[frame].fenceWaitValue; }
        DXGI_SWAP_CHAIN_DESC1 GetSwapChainDescription() const { return TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get()); }
        Window& GetWindow() const { return *m_window; }

    private:
        void Reset();
        FrameData& CurrentFrameData() { return m_frameData[m_swapChain->GetCurrentBackBufferIndex()]; }

    public:
        TypedD3D::D3D12::Device5 GetDevice() const { return m_device; }
    };



    template<class DrawCallback>
    class Renderer : public Backend
    {
        DrawCallback m_drawCallback;

    public:
        template<class... Args>
        Renderer(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, Args&&... args) :
            Backend(window, factory, device),
            m_drawCallback(static_cast<Backend&>(*this), std::forward<Args>(args)...)
        {
        }

    private:
        void Draw() final
        {
            m_drawCallback.Draw();
        }

        std::any GetRenderer() final { return &m_drawCallback; }
    };



    struct DefaultDraw
    {
        gsl::strict_not_null<Backend*> m_renderer;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        MeshObject m_mesh;

    public:
        DefaultDraw(Backend& renderer);

    public:
        void Draw();
    };
}