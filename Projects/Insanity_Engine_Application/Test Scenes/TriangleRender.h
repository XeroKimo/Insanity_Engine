#pragma once


namespace InsanityEngine
{
    namespace DX11
    {
        class Device;
    }

    namespace Application
    {
        class Window;
    }
}


extern void TriangleRenderSetup(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window);
extern void TriangleRenderUpdate(float dt);
extern void TriangleRender(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window);
extern void ShutdownTriangleRender();