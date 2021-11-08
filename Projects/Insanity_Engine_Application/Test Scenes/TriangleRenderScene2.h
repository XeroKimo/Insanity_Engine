#pragma once
#include "SDL.h"

namespace InsanityEngine
{
    namespace DX11
    {
        class Device;
        class Window;
        namespace StaticMesh
        {
            class Renderer;
        }
    }

}

extern void TriangleRenderSetup2(InsanityEngine::DX11::Device& device, InsanityEngine::DX11::StaticMesh::Renderer& renderer, InsanityEngine::DX11::Window& window);
extern void TriangleRenderInput2(SDL_Event event);
extern void TriangleRenderUpdate2(float dt);
extern void TriangleRenderShutdown2();