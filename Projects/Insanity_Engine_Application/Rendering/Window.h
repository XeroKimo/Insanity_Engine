#pragma once
#include "SDL.h"
#include "SDL_syswm.h"
#include "Insanity_Math.h"
#include "TypedD3D.h"
#include "d3d11/Backend.h"
#include "d3d12/Backend.h"
#include <memory>
#include <string_view>
#include <any>
#include <Windows.h>
#include <gsl/gsl>

namespace InsanityEngine::Rendering
{
    class Window
    {
    private:
        using WindowHandle = std::unique_ptr<SDL_Window, decltype([](SDL_Window* w) { SDL_DestroyWindow(w); })> ;

        template<class DrawCallback>
        struct RendererTag {};

    private:
        WindowHandle m_windowHandle;
        std::unique_ptr<BackendInterface> m_backEnd = nullptr;

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
            return Window(title, windowPosition, windowSize, windowFlags, factory, device, deviceContext, RendererTag<DrawCallback>(), std::forward<Args>(args)...);
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
            return Window(title, windowPosition, windowSize, windowFlags, factory, device, RendererTag<DrawCallback>(), std::forward<Args>(args)...);
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
            RendererTag<DrawCallback>,
            Args&&... args) :
            m_windowHandle(SDL_CreateWindow(
                title.data(),
                windowPosition.x(),
                windowPosition.y(),
                windowSize.x(),
                windowSize.y(),
                windowFlags)),
            m_backEnd(std::make_unique<D3D11::Renderer<DrawCallback>>(*this, factory, device, deviceContext, std::forward<Args>(args)...))
        {

        }

        template<class DrawCallback, class... Args>
        Window(std::string_view title,
            InsanityEngine::Math::Types::Vector2i windowPosition,
            InsanityEngine::Math::Types::Vector2i windowSize,
            Uint32 windowFlags,
            IDXGIFactory2& factory,
            TypedD3D::D3D12::Device5 device,
            RendererTag<DrawCallback>,
            Args&&... args) :
            m_windowHandle(SDL_CreateWindow(
                title.data(),
                windowPosition.x(),
                windowPosition.y(),
                windowSize.x(),
                windowSize.y(),
                windowFlags)),
            m_backEnd(std::make_unique<D3D12::Renderer<DrawCallback>>(*this, factory, device, std::forward<Args>(args)...))
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
}