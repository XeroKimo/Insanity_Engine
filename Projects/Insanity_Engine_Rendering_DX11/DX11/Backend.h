#pragma once
#include "TypedD3D12.h"
#include "Insanity_Math.h"
#include "TypedD3D11.h"
#include "TypedDXGI.h"
#include <gsl/gsl>

namespace InsanityEngine::Rendering::D3D11
{
    struct BackendInitParams
    {
        TypedD3D::Wrapper<ID3D11Device> device;
        TypedD3D::Wrapper<ID3D11DeviceContext> deviceContext;
        TypedD3D::Wrapper<IDXGIFactory2> factory;
        HWND windowHandle;
        Math::Types::Vector2ui windowSize;
        UINT bufferCount = 2;
        DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    };

    class Backend
    {
        template<class T>
        using Wrapper = TypedD3D::Wrapper<T>;

    private:
        gsl::strict_not_null<Wrapper<ID3D11Device>> m_device;
        gsl::strict_not_null<Wrapper<ID3D11DeviceContext>> m_deviceContext;
        gsl::strict_not_null<Wrapper<IDXGISwapChain3>> m_swapChain;
        Wrapper<ID3D11RenderTargetView> m_backBuffer;

    public:
        Backend(const BackendInitParams& params);

        ~Backend();

    public:
        void ResizeBuffers(Math::Types::Vector2ui size);
        void SetFullscreen(bool fullscreen);
        void SetWindowSize(Math::Types::Vector2ui size);

    public:
        bool IsFullscreen() const
        {
            return m_swapChain->GetFullscreenState().first;
        }
        Math::Types::Vector2ui GetWindowSize() const
        {
            DXGI_SWAP_CHAIN_DESC1 description = m_swapChain->GetDesc1();
            return { description.Width, description.Height };
        }


    public:
        gsl::not_null<Wrapper<ID3D11RenderTargetView>> GetBackBufferResource() const { return m_backBuffer; }
        void Present();

    public:
        gsl::not_null<Wrapper<ID3D11Device>> GetDevice() const { return m_device; }
        gsl::not_null<Wrapper<ID3D11DeviceContext>> GetDeviceContext() const { return m_deviceContext; }

    private:
        Wrapper<ID3D11RenderTargetView> CreateBackBuffer();
    };

    template<class Renderer>
    class BackendWithRenderer : public Backend
    {
    private:
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
        TypedD3D::Wrapper<ID3D11VertexShader> m_vertexShader;
        TypedD3D::Wrapper<ID3D11PixelShader> m_pixelShader;
        TypedD3D::Wrapper<ID3D11InputLayout> m_inputLayout;
        TypedD3D::Wrapper<ID3D11Buffer> m_vertexBuffer;

    public:
        DefaultDraw(Backend& backend);

    public:
        void Draw(Backend& backend);
    };
}