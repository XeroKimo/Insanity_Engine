#pragma once
#include "SDL.h"
#include "SDL_syswm.h"
#include "Insanity_Math.h"
#include "TypedD3D.h"
#include <d3d11_4.h>
#include <memory>
#include <string_view>
#include <any>
#include <Windows.h>
#include <gsl/gsl>

namespace InsanityEngine::Rendering
{
    template<class DrawCallback>
    struct RendererTag {};

    class Window
    {
    private:
        template<class Callback>
        struct CallbackTag
        {

        };

    public:
        class BackEnd
        {
            friend class Window;

        public:
            virtual ~BackEnd() = default;

        private:
            virtual void Draw() = 0;
            virtual void ResizeBuffers(Math::Types::Vector2ui size) = 0;
            virtual void SetFullscreen(bool fullscreen) = 0;
            virtual void SetWindowSize(Math::Types::Vector2ui size) = 0;
            virtual std::any GetRenderer() = 0;

        public:
            virtual bool IsFullscreen() const = 0;
            virtual Math::Types::Vector2ui GetWindowSize() const = 0;
        };

        class Null : public BackEnd
        {
        private:
            Window* m_window;

        public:
            Null(Window& window) :
                m_window(&window)
            {

            }

        private:
            void Draw() final {}
            void ResizeBuffers(Math::Types::Vector2ui size) final {}
            void SetFullscreen(bool fullscreen) {}
            void SetWindowSize(Math::Types::Vector2ui size) final {}
            bool IsFullscreen() const final {}
            Math::Types::Vector2ui GetWindowSize() const final {}

        };

        class DirectX11 : public BackEnd
        {
        private:
            Microsoft::WRL::ComPtr<ID3D11Device5> m_device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_deviceContext;
            Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBuffer;

        public:
            DirectX11(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext);
            ~DirectX11();

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
            ID3D11RenderTargetView& GetBackBufferResource() const { return *m_backBuffer.Get(); }
            void Present();

        public:
            Microsoft::WRL::ComPtr<ID3D11Device5> GetDevice() const { return m_device; }
            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> GetDeviceContext() const { return m_deviceContext; }
        };

        class DirectX12 : public BackEnd
        {
        private:
            struct FrameData
            {
                size_t idleAllocatorIndex = 0;
                std::vector<TypedD3D::D3D12::CommandAllocator::Direct> allocators;
                UINT64 fenceWaitValue;
            };

        private:
            TypedD3D::D3D12::Device5 m_device;
            TypedD3D::D3D12::CommandQueue::Direct m_mainQueue;
            Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
            TypedD3D::D3D12::DescriptorHeap::RTV m_swapChainDescriptorHeap;
            Microsoft::WRL::ComPtr<ID3D12Fence> m_mainFence;
            UINT64 m_mainFenceWaitValue = 0;
            std::vector<FrameData> m_frameData;

        public:
            DirectX12(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device);
            ~DirectX12();

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
            UINT64 GetCurrentFenceValue() const { return m_mainFenceWaitValue; }
            UINT64 GetFrameFenceValue(size_t frame) const { return m_frameData[frame].fenceWaitValue; }
            DXGI_SWAP_CHAIN_DESC1 GetSwapChainDescription() const { return TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get()); }

        private:
            void Reset();

        public:
            TypedD3D::D3D12::Device5 GetDevice() const { return m_device; }
        };

    private:
        template<class DrawCallback>
        class DirectX11Renderer : public DirectX11
        {
            DrawCallback m_drawCallback;

        public:
            template<class... Args>
            DirectX11Renderer(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext, Args&&... args) :
                DirectX11(window, factory, device, deviceContext),
                m_drawCallback(static_cast<DirectX11&>(*this), std::forward<Args>(args)...)
            {
            }

        private:
            void Draw() final
            {
                m_drawCallback.Draw();
            }

            std::any GetRenderer() final { return &m_drawCallback; }
        };

        template<class DrawCallback>
        class DirectX12Renderer : public DirectX12
        {
            DrawCallback m_drawCallback;

        public:
            template<class... Args>
            DirectX12Renderer(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device, Args&&... args) :
                DirectX12(window, factory, device),
                m_drawCallback(static_cast<DirectX12&>(*this), std::forward<Args>(args)...)
            {
            }

        private:
            void Draw() final
            {
                m_drawCallback.Draw();
            }

            std::any GetRenderer() final { return &m_drawCallback; }
        };

    private:
        using WindowHandle = std::unique_ptr < SDL_Window, decltype([](SDL_Window* w) { SDL_DestroyWindow(w); }) > ;

    private:
        WindowHandle m_windowHandle;
        std::unique_ptr<BackEnd> m_backEnd = nullptr;

    public:
        template<class DrawCallback, class... Args>
        static Window Create(std::string_view title,
            InsanityEngine::Math::Types::Vector2i windowPosition,
            InsanityEngine::Math::Types::Vector2i windowSize,
            Uint32 windowFlags,
            IDXGIFactory2& factory,
            Microsoft::WRL::ComPtr<ID3D11Device5> device,
            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext,
            Args&&... args)
        {
            return Window(title, windowPosition, windowSize, windowFlags, factory, device, deviceContext, CallbackTag<DrawCallback>(), std::forward<Args>(args)...);
        }

        template<class DrawCallback, class... Args>
        static Window Create(std::string_view title,
            InsanityEngine::Math::Types::Vector2i windowPosition,
            InsanityEngine::Math::Types::Vector2i windowSize,
            Uint32 windowFlags,
            IDXGIFactory2& factory,
            TypedD3D::D3D12::Device5 device,
            Args&&... args)
        {
            return Window(title, windowPosition, windowSize, windowFlags, factory, device, CallbackTag<DrawCallback>(), std::forward<Args>(args)...);
        }

    private:
        template<class DrawCallback, class... Args>
        Window(std::string_view title,
            InsanityEngine::Math::Types::Vector2i windowPosition,
            InsanityEngine::Math::Types::Vector2i windowSize,
            Uint32 windowFlags,
            IDXGIFactory2& factory,
            Microsoft::WRL::ComPtr<ID3D11Device5> device,
            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext,
            CallbackTag<DrawCallback>,
            Args&&... args) :
            m_windowHandle(SDL_CreateWindow(
                title.data(),
                windowPosition.x(),
                windowPosition.y(),
                windowSize.x(),
                windowSize.y(),
                windowFlags)),
            m_backEnd(std::make_unique<DirectX11Renderer<DrawCallback>>(*this, factory, device, deviceContext, std::forward<Args>(args)...))
        {

        }

        template<class DrawCallback, class... Args>
        Window(std::string_view title,
            InsanityEngine::Math::Types::Vector2i windowPosition,
            InsanityEngine::Math::Types::Vector2i windowSize,
            Uint32 windowFlags,
            IDXGIFactory2& factory,
            TypedD3D::D3D12::Device5 device,
            CallbackTag<DrawCallback>,
            Args&&... args) :
            m_windowHandle(SDL_CreateWindow(
                title.data(),
                windowPosition.x(),
                windowPosition.y(),
                windowSize.x(),
                windowSize.y(),
                windowFlags)),
            m_backEnd(std::make_unique<DirectX12Renderer<DrawCallback>>(*this, factory, device, std::forward<Args>(args)...))
        {

        }

    public:
        void HandleEvent(const SDL_Event& event);

        void Draw() { m_backEnd->Draw(); }

        void SetFullscreen(bool fullscreen)
        {
            m_backEnd->SetFullscreen(fullscreen);
        }

        void SetWindowSize(Math::Types::Vector2ui size)
        {
            m_backEnd->SetWindowSize(size);
        }

        bool IsFullscreen() const
        {
            return m_backEnd->IsFullscreen();
        }

        Math::Types::Vector2ui GetWindowSize() const
        {
            return m_backEnd->GetWindowSize();
        }

        template<class Ty>
        Ty* GetRenderer() const
        {
            std::any renderer = m_backEnd->GetRenderer();
            if(renderer.type() == typeid(Ty*))
                return std::any_cast<Ty*>(renderer);
            return nullptr;
        }

    public:
        SDL_Window& GetWindow() { return *m_windowHandle; }
        const SDL_Window& GetWindow() const { return *m_windowHandle; }
        HWND GetWindowHandle() const
        {
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            SDL_GetWindowWMInfo(m_windowHandle.get(), &info);
            return info.info.win.window;
        }
    };

    namespace D3D11
    {
        struct DefaultDraw
        {
            gsl::strict_not_null<Window::DirectX11*> m_renderer;
            Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
            Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;

        public:
            DefaultDraw(Window::DirectX11& renderer);

        public:
            void Draw();
        };
    }

    namespace D3D12
    {
        struct DefaultDraw
        {
            gsl::strict_not_null<Window::DirectX12*> m_renderer;
            TypedD3D::D3D12::CommandList::Direct5 m_commandList;

            Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
            TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
            Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        public:
            DefaultDraw(Window::DirectX12& renderer);

        public:
            void Draw();
        };

        struct NullDraw
        {
        public:
            NullDraw() = default;

        public:
            void Draw() {}
        };
    }
}