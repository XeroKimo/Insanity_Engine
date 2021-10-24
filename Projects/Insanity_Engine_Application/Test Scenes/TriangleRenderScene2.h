#pragma once
#include "SDL.h"

namespace InsanityEngine
{
    namespace DX11
    {
        class Device;
    }

    namespace Application
    {
        class Window;
        class Renderer;
    }
}

extern void TriangleRenderSetup2(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Renderer& renderer, InsanityEngine::Application::Window& window);
extern void TriangleRenderInput2(SDL_Event event);
extern void TriangleRenderUpdate2(float dt);
extern void TriangleRenderShutdown2();