#pragma once
#include "Insanity_Math.h"
#include <string_view>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <memory>
#include <gsl/pointers>

namespace InsanityEngine::Rendering
{
    class WindowBase
    {
        using WindowHandle = std::unique_ptr<SDL_Window, decltype([](SDL_Window* w) { SDL_DestroyWindow(w); })>;

    private:
        WindowHandle m_windowHandle;

    public:
        WindowBase(std::string_view title,
            Math::Types::Vector2i position,
            Math::Types::Vector2i size,
            Uint32 flags) :
            m_windowHandle(SDL_CreateWindow(
                title.data(),
                position.x(),
                position.y(),
                size.x(),
                size.y(),
                flags))
        {

        }

    public:
        gsl::not_null<SDL_Window*> GetHandle() const { return m_windowHandle.get(); }
        HWND GetRawHandle() const;
    };

    template<class BackendTy>
    class Window : public WindowBase, public BackendTy
    {
    public:
        template<class... BackendParams>
        Window(std::string_view title,
            Math::Types::Vector2i position,
            Math::Types::Vector2i size,
            Uint32 flags,
            BackendParams&&... params) :
            WindowBase(title, position, size, flags),
            BackendTy(GetRawHandle(), size, std::forward<BackendParams>(params)...)
        {

        }
    };
}