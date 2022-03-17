#include "Window.h"
#include "d3dx12.h"

namespace InsanityEngine::Rendering
{
    static constexpr UINT frameCount = 2;
    Window::DirectX12::DirectX12(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device) :
        m_device(device),
        m_mainQueue(device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(
            D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0).GetValue()),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory,
            *m_mainQueue.Get(),
            std::any_cast<HWND>(window.GetWindowHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            frameCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue()),
        m_swapChainDescriptorHeap(device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>(frameCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0).GetValue()),
        m_mainFence(device->CreateFence(0, D3D12_FENCE_FLAG_NONE).GetValue()),
        m_mainFenceWaitValue(0)
    {
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
        m_frameData.reserve(5);
        m_frameData.resize(frameCount);
    }

    Window::DirectX12::~DirectX12()
    {
        Reset();
        SetFullscreen(false);
    }

    void Window::DirectX12::ResizeBuffers(Math::Types::Vector2ui size)
    {
        Reset();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->ResizeBuffers(frameCount, size.x(), size.y(), desc.Format, desc.Flags);
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*m_device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
    }

    void Window::DirectX12::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        Reset();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(frameCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*m_device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
    }

    void Window::DirectX12::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = swapChainDesc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(&modeDesc);
    }

    bool Window::DirectX12::IsFullscreen() const
    {
        BOOL isFullscreen;
        m_swapChain->GetFullscreenState(&isFullscreen, nullptr);
        return isFullscreen;
    }

    void Window::DirectX12::SignalQueue()
    {
        m_frameData[m_swapChain->GetCurrentBackBufferIndex()].fenceWaitValue = m_mainFenceWaitValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*m_mainQueue.Get(), *m_mainFence.Get(), m_mainFenceWaitValue);
    }

    void Window::DirectX12::WaitForCurrentFrame()
    {
        FrameData& currentFrame = m_frameData[GetCurrentBackBufferIndex()];
        TypedD3D::Helpers::D3D12::StallCPUThread(*m_mainFence.Get(), currentFrame.fenceWaitValue);
        currentFrame.idleAllocatorIndex = 0;
    }

    void Window::DirectX12::Present()
    {
        m_swapChain->Present(1, 0);
    }

    TypedD3D::D3D12::DescriptorHandle::CPU_RTV Window::DirectX12::GetBackBufferHandle()
    {
        UINT stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        TypedD3D::D3D12::DescriptorHandle::CPU_RTV handle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handle.Ptr() += stride * m_swapChain->GetCurrentBackBufferIndex();
        return handle;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> Window::DirectX12::GetBackBufferResource()
    {
        return TypedD3D::Helpers::DXGI::SwapChain::GetBuffer(*m_swapChain.Get(), GetCurrentBackBufferIndex()).GetValue();
    }

    TypedD3D::D3D12::CommandAllocator::Direct Window::DirectX12::CreateOrGetAllocator()
    {
        FrameData& currentFrame = m_frameData[GetCurrentBackBufferIndex()];
        TypedD3D::D3D12::CommandAllocator::Direct allocator;
        if(currentFrame.idleAllocatorIndex == currentFrame.allocators.size())
        {
            currentFrame.allocators.push_back(m_device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>().GetValue());
            allocator = currentFrame.allocators.back();
        }
        else
        {
            allocator = currentFrame.allocators[currentFrame.idleAllocatorIndex];
            allocator->Reset();
        }
        ++currentFrame.idleAllocatorIndex;

        return allocator;
    }

    UINT Window::DirectX12::GetCurrentBackBufferIndex() const
    {
        return m_swapChain->GetCurrentBackBufferIndex();
    }

    void Window::DirectX12::Reset()
    {
        TypedD3D::Helpers::D3D12::FlushCommandQueue(*m_mainQueue.Get(), *m_mainFence.Get(), m_mainFenceWaitValue);

        TypedD3D::Helpers::D3D12::ResetFence(*m_mainFence.Get());
        m_mainFenceWaitValue = 0;
        for(FrameData& frame : m_frameData)
        {
            frame.fenceWaitValue = 0;
        }
    }

    void Window::HandleEvent(const SDL_Event& event)
    {
        if(event.type == SDL_WINDOWEVENT)
        {
            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                m_backEnd->ResizeBuffers({ event.window.data1, event.window.data2 });
            }
        }
    }

    namespace D3D12
    {
        DefaultDraw::DefaultDraw(TypedD3D::D3D12::Device5 device) :
            m_device(device),
            m_commandList(device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>())
        {
        }

        void DefaultDraw::Draw(Window::DirectX12& renderer)
        {
            using Microsoft::WRL::ComPtr;
            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);

            ComPtr<ID3D12Resource> backBuffer = renderer.GetBackBufferResource();
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->ClearRenderTargetView(renderer.GetBackBufferHandle(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }), {});

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(submitList));
            renderer.SignalQueue();
            renderer.Present();
            renderer.WaitForCurrentFrame();
        }
    }
}