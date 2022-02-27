#pragma once
#include "TypedD3D.h"

namespace InsanityEngine::Rendering
{
    class WindowInterface;

    class RendererInterface
    {
        friend class PolymorphicRenderer;
    private:
        virtual void Draw() = 0;
    };

    namespace D3D12
    {
        class DrawCallback;

        class Renderer : public RendererInterface
        {
        private:
            struct Frame
            {
                size_t idleAllocatorIndex = 0;
                std::vector<TypedD3D::D3D12::CommandAllocator::Direct> allocators;
                UINT64 fenceValue;
            };

        public:
            TypedD3D::D3D12::Device5 device;
            TypedD3D::D3D12::CommandQueue::Direct mainQueue;
            Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
            
        private:
            Microsoft::WRL::ComPtr<ID3D12Fence> m_mainFence;
            std::vector<Frame> m_frames;
            DrawCallback* m_drawCallback;

        public:
            Renderer(WindowInterface& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, DrawCallback& drawCallback);

        private:
            void Draw() final;

        public:
            void SignalQueue();
            void WaitForNextFrame();
            TypedD3D::D3D12::CommandAllocator::Direct GetAllocator();
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