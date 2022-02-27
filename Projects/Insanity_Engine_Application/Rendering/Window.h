#pragma once
#include "SDL.h"
#include "SDL_syswm.h"
#include "Insanity_Math.h"
#include <memory>
#include <string_view>
#include <any>
#include <Windows.h>

namespace InsanityEngine::Rendering
{
    class WindowInterface
    {
    public:
        virtual Math::Types::Vector2i GetSize() const = 0;
        virtual std::any GetHandle() const = 0;
    };

    struct NullRenderer 
    {
    public:
        NullRenderer(WindowInterface& window) {}
    };

    template<class Renderer>
    class Window : public WindowInterface
    {
        using WindowHandle = std::unique_ptr<SDL_Window, decltype([](SDL_Window* w) { SDL_DestroyWindow(w); })>;

    private:
        WindowHandle m_windowHandle;
        Renderer m_renderer;

    public:
        template<class... RendererParams>
        Window(std::string_view title, 
            InsanityEngine::Math::Types::Vector2i windowPosition, 
            InsanityEngine::Math::Types::Vector2i windowSize, 
            Uint32 flags,
            RendererParams&&... params) :
            m_windowHandle(SDL_CreateWindow(
                title.data(), 
                windowPosition.x(), 
                windowPosition.y(), 
                windowSize.x(), 
                windowSize.y(), 
                flags)),
            m_renderer(*static_cast<WindowInterface*>(this), params...)
        {

        }

    public:
        Math::Types::Vector2i GetSize() const final
        {
            Math::Types::Vector2i windowSize;

            SDL_GetWindowSize(m_windowHandle.get(), &windowSize.x(), &windowSize.y());

            return windowSize;
        }

        std::any GetHandle() const final
        {
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            SDL_GetWindowWMInfo(m_windowHandle.get(), &info);
            return info.info.win.window;
        }

    public:
        SDL_Window& GetWindow() { return *m_windowHandle; }
        const SDL_Window& GetWindow() const { return *m_windowHandle; }

        Renderer& GetRenderer() { return m_renderer; }
        const Renderer& GetRenderer() const { return m_renderer; }
    };

    using DefaultWindow = Window<NullRenderer>;
}