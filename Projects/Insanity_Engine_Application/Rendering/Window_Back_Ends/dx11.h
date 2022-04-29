#pragma once
#include "Backend.h"
#include "TypedD3D.h"
#include "Insanity_Math.h"
#include <wrl/client.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <gsl/gsl>

namespace InsanityEngine::Rendering
{
    class Window;
}

namespace InsanityEngine::Rendering::D3D11
{
    class Backend : public BackendInterface
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    private:
        ComPtr<ID3D11Device5> m_device;
        ComPtr<ID3D11DeviceContext4> m_deviceContext;
        ComPtr<IDXGISwapChain4> m_swapChain;
        ComPtr<ID3D11RenderTargetView> m_backBuffer;

    public:
        Backend(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext); 
        ~Backend();

    private:
        void ResizeBuffers(Math::Types::Vector2ui size) final;
        void SetFullscreen(bool fullscreen) final;
        void SetWindowSize(Math::Types::Vector2ui size) final;

    public:
        bool IsFullscreen() const final
        {
            BOOL isFullscreen;
            m_swapChain->GetFullscreenState(&isFullscreen, nullptr);
            return isFullscreen;
        }
        Math::Types::Vector2ui GetWindowSize() const final
        {
            DXGI_SWAP_CHAIN_DESC1 description = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
            return { description.Width, description.Height };
        }


    public:
        ID3D11RenderTargetView& GetBackBufferResource() const { return *m_backBuffer.Get(); }
        void Present();

    public:
        Microsoft::WRL::ComPtr<ID3D11Device5> GetDevice() const { return m_device; }
        Microsoft::WRL::ComPtr<ID3D11DeviceContext4> GetDeviceContext() const { return m_deviceContext; }
    };

    template<class DrawCallback>
    class Renderer : public Backend
    {
        DrawCallback m_drawCallback;

    public:
        template<class... Args>
        Renderer(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext, Args&&... args) :
            Backend(window, factory, device, deviceContext),
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
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;

    public:
        DefaultDraw(Backend& renderer);

    public:
        void Draw();
    };
}