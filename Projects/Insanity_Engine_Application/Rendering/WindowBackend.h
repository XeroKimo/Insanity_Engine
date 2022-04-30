#pragma once
#include "Insanity_Math.h"
#include <any>

namespace InsanityEngine::Rendering
{
    class BackendInterface
    {
        friend class Window;

    private:
        virtual void ResizeBuffers(Math::Types::Vector2ui size) = 0;
        virtual void SetFullscreen(bool fullscreen) = 0;
        virtual void SetWindowSize(Math::Types::Vector2ui size) = 0;

        virtual void Draw() = 0;
        virtual std::any GetRenderer() = 0;

    public:
        virtual bool IsFullscreen() const = 0;
        virtual Math::Types::Vector2ui GetWindowSize() const = 0;
    };
};