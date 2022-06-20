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
    private:
        struct FrameData
        {
            size_t idleAllocatorIndex = 0;
            std::vector<TypedD3D::D3D12::CommandAllocator::Direct> allocators;
            UINT64 fenceWaitValue;
        };

    private:
        gsl::not_null<TypedD3D::Wrapper<ID3D12Device5>> m_device;
        gsl::not_null<TypedD3D::Direct<ID3D12CommandQueue>> m_mainQueue;
        gsl::not_null<TypedD3D::Wrapper<IDXGISwapChain3>> m_swapChain;
        gsl::not_null<TypedD3D::RTV<ID3D12DescriptorHeap>> m_swapChainDescriptorHeap;
        ComPtr<ID3D12Fence> m_mainFence;
        UINT64 m_previousFrameFenceValue = 0;
        std::vector<FrameData> m_frameData;

    public:
        Backend(const BackendInitParams& params);
        ~Backend();

    private:
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
        void SignalQueue();
        void WaitForCurrentFrame();
        void Present();

        template<size_t Extents>
        void ExecuteCommandLists(std::span<TypedD3D::Direct<ID3D12CommandList>, Extents> commandLists)
        {
            m_mainQueue->ExecuteCommandLists(commandLists);
        }

        TypedD3D::D3D12::CommandAllocator::Direct CreateOrGetAllocator();
        TypedD3D::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> GetBackBufferHandle();
        TypedD3D::Wrapper<ID3D12Resource> GetBackBufferResource();

    public:
        UINT GetCurrentBackBufferIndex() const;
        UINT64 GetCurrentFenceValue() const { return GetFrameFenceValue(m_swapChain->GetCurrentBackBufferIndex()); }
        UINT64 GetFrameFenceValue(size_t frame) const { return m_frameData[frame].fenceWaitValue; }
        DXGI_SWAP_CHAIN_DESC1 GetSwapChainDescription() const { return m_swapChain->GetDesc1(); }

    private:
        void Reset();
        FrameData& CurrentFrameData() { return m_frameData[m_swapChain->GetCurrentBackBufferIndex()]; }

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
            m_renderer.Draw(static_cast<Backend&>(*this));
        }

        Renderer& GetRenderer() { return m_renderer; }
        const Renderer& GetRenderer() const { return m_renderer; }
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
        void Draw(Backend& backend);
    };
}