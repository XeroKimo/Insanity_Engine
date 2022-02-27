#include "Renderer.h"
#include "Window.h"

namespace InsanityEngine::Rendering::D3D12
{
    static constexpr UINT frameCount = 2;

    Renderer::Renderer(WindowInterface& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, DrawCallback& drawCallback) :
        device(device),
        mainQueue(device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(
            D3D12_COMMAND_QUEUE_PRIORITY_HIGH, 
            D3D12_COMMAND_QUEUE_FLAG_NONE, 
            0).GetValue()),
        swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory, 
            *mainQueue.Get(),
            std::any_cast<HWND>(window.GetHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            frameCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue()),
        m_drawCallback(&drawCallback),
        m_mainFence(device->CreateFence(0, D3D12_FENCE_FLAG_NONE).GetValue()),
        m_frames(frameCount)
    {
        m_drawCallback->Initialize(*this);
    }

    void Renderer::Draw()
    {
        m_drawCallback->Draw(*this);
    }

    void Renderer::SignalQueue()
    {
        Frame& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
        currentFrame.fenceValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*mainQueue.Get(), *m_mainFence.Get(), currentFrame.fenceValue);
    }

    void Renderer::WaitForNextFrame()
    {
        Frame& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
        TypedD3D::Helpers::D3D12::StallCPUThread(*m_mainFence.Get(), currentFrame.fenceValue);
        currentFrame.idleAllocatorIndex = 0;
    }

    TypedD3D::D3D12::CommandAllocator::Direct Renderer::GetAllocator()
    {
        Frame& currentFrame = m_frames[swapChain->GetCurrentBackBufferIndex()];
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

    void DefaultDraw::Initialize(Renderer& renderer)
    {
        m_commandList = renderer.device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>();
    }

    void DefaultDraw::Draw(Renderer& renderer)
    {
        m_commandList->Reset(renderer.GetAllocator(), nullptr);
        m_commandList->Close();

        auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
        renderer.mainQueue->ExecuteCommandLists(std::span(submitList));
        renderer.SignalQueue();

        renderer.swapChain->Present(1, 0);
        renderer.WaitForNextFrame();
    }
}
