#pragma once
#include "TypedD3D.h"

namespace InsanityEngine::Rendering
{
    class WindowInterface;

    class RendererInterface
    {
    private:
        virtual void Draw() = 0;

    public:
        friend class PolymorphicRenderer;
    };

    namespace D3D12
    {
        class DrawCallback;

        class Renderer : public RendererInterface
        {
        private:
            struct FrameData
            {
                size_t idleAllocatorIndex = 0;
                std::vector<TypedD3D::D3D12::CommandAllocator::Direct> allocators;
                UINT64 fenceWaitValue;
            };

        public:
            TypedD3D::D3D12::Device5 device;

        private:
            TypedD3D::D3D12::CommandQueue::Direct m_mainQueue;

        public:
            Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
            
        private:
            TypedD3D::D3D12::DescriptorHeap::RTV m_swapChainDescriptorHeap;

            Microsoft::WRL::ComPtr<ID3D12Fence> m_mainFence;
            UINT64 m_mainFenceWaitValue = 0;
            std::vector<FrameData> m_frames;
            DrawCallback* m_drawCallback;

        public:
            Renderer(WindowInterface& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, DrawCallback& drawCallback);
            ~Renderer();
        private:
            void Draw() final;

        public:
            void SignalQueue();
            void WaitForCurrentFrame();

            template<size_t Extents>
            void ExecuteCommandLists(std::span<TypedD3D::D3D12::CommandList::Direct, Extents> commandLists)
            {
                m_mainQueue->ExecuteCommandLists(commandLists);
            }
            TypedD3D::D3D12::CommandAllocator::Direct GetAllocator();

            TypedD3D::D3D12::DescriptorHandle::CPU_RTV GetBackBufferHandle();
        };

        class DrawCallback
        {
        public:
            virtual void Initialize(Renderer& renderer) = 0;
            virtual void Draw(Renderer& renderer) = 0;
        };

        class DefaultDraw : public DrawCallback
        {
        private:
            TypedD3D::D3D12::CommandList::Direct5 m_commandList;

        public:
            void Initialize(Renderer& renderer) final;
            void Draw(Renderer& renderer) final;
        };
    }

    class PolymorphicRenderer
    {
    private:
        std::unique_ptr<RendererInterface> m_renderer;

    public:
        PolymorphicRenderer(WindowInterface& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, D3D12::DrawCallback& drawCallback) :
            m_renderer(std::make_unique<D3D12::Renderer>(window, factory, device, drawCallback))
        {

        }

    public:
        void Draw() { m_renderer->Draw(); } 
    };

}