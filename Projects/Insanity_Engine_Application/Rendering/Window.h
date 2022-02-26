#pragma once
#include "SDL.h"
#include "Insanity_Math.h"
#include <memory>
#include <string_view>

namespace InsanityEngine::Rendering
{
    struct NullRenderer 
    {
    public:
        NullRenderer(SDL_Window& window) {}
    };

    template<class Renderer>
    class Window
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
            RendererParams... params) :
            m_windowHandle(SDL_CreateWindow(
                title.data(), 
                windowPosition.x(), 
                windowPosition.y(), 
                windowSize.x(), 
                windowSize.y(), 
                flags)),
            m_renderer(*m_windowHandle, params...)
        {

        }

    public:
        SDL_Window& GetWindow() { return *m_windowHandle; }
        const SDL_Window& GetWindow() const { return *m_windowHandle; }

        Renderer& GetRenderer() { return m_renderer; }
        const Renderer& GetRenderer() const { return m_renderer; }
    };

    using DefaultWindow = Window<NullRenderer>;
}