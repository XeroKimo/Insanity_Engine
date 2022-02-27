#include "Renderer.h"
#include "Window.h"
#include "d3dx12.h"

namespace InsanityEngine::Rendering::D3D12
{
    static constexpr UINT frameCount = 2;

    Renderer::Renderer(WindowInterface& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, DrawCallback& drawCallback) :
        device(device),
        m_mainQueue(device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(
            D3D12_COMMAND_QUEUE_PRIORITY_HIGH, 
            D3D12_COMMAND_QUEUE_FLAG_NONE, 
            0).GetValue()),
        swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory, 
            *m_mainQueue.Get(),
            std::any_cast<HWND>(window.GetHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            frameCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue()),
        m_drawCallback(&drawCallback),
        m_swapChainDescriptorHeap(device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>(frameCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0).GetValue()),
        m_mainFence(device->CreateFence(0, D3D12_FENCE_FLAG_NONE).GetValue()),
        m_frames(frameCount)
    {
        m_drawCallback->Initialize(*this);
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*device.Get(), *swapChain.Get(), *m_swapChainDescriptorHeap.Get());
    }

    Renderer::~Renderer()
    {
        WaitForCurrentFrame();
        swapChain->SetFullscreenState(false, nullptr);
    }

    void Renderer::Draw()
    {
        m_drawCallback->Draw(*this);
    }

    void Renderer::SignalQueue()
    {
        FrameData& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
        currentFrame.fenceWaitValue = m_mainFenceWaitValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*m_mainQueue.Get(), *m_mainFence.Get(), m_mainFenceWaitValue);
    }

    void Renderer::WaitForCurrentFrame()
    {
        FrameData& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
        TypedD3D::Helpers::D3D12::StallCPUThread(*m_mainFence.Get(), currentFrame.fenceWaitValue);
        currentFrame.idleAllocatorIndex = 0;
    }

    TypedD3D::D3D12::CommandAllocator::Direct Renderer::GetAllocator()
    {
        FrameData& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
        TypedD3D::D3D12::CommandAllocator::Direct allocator;
        if(currentFrame.idleAllocatorIndex == currentFrame.allocators.size())
        {
            currentFrame.allocators.push_back(device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>().GetValue());
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

    TypedD3D::D3D12::DescriptorHandle::CPU_RTV Renderer::GetBackBufferHandle()
    {
        UINT stride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        TypedD3D::D3D12::DescriptorHandle::CPU_RTV handle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handle.Ptr() += stride * swapChain->GetCurrentBackBufferIndex();
        return handle;
    }

    void DefaultDraw::Initialize(Renderer& renderer)
    {
        m_commandList = renderer.device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>();
    }

    void DefaultDraw::Draw(Renderer& renderer)
    {
        using Microsoft::WRL::ComPtr;
        m_commandList->Reset(renderer.GetAllocator(), nullptr);

        ComPtr<ID3D12Resource> backBuffer = TypedD3D::Helpers::DXGI::SwapChain::GetBuffer(*renderer.swapChain.Get(), renderer.swapChain->GetCurrentBackBufferIndex()).GetValue();
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(std::span(&barrier, 1));
        m_commandList->ClearRenderTargetView(renderer.GetBackBufferHandle(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }), {});

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
        m_commandList->ResourceBarrier(std::span(&barrier, 1));
        m_commandList->Close();

        auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
        renderer.ExecuteCommandLists(std::span(submitList));
        renderer.SignalQueue();
        renderer.swapChain->Present(1, 0);
        renderer.WaitForCurrentFrame();
    }
}
