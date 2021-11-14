#pragma once
#include <memory>

namespace InsanityEngine
{
    class ResourceFactory;
    class ComponentFactory;
    namespace DX11
    {
        class Device;
        class Window;
        class RenderModule;
        namespace StaticMesh
        {
            class Renderer;
        }
    }
}

namespace InsanityEngine::Application
{

    class Application
    {
    private:
        DX11::Device& m_device;
        DX11::Window& m_window;
        //DX11::StaticMesh::Renderer& m_renderer;
        DX11::RenderModule& m_renderer;
        ResourceFactory& m_factory;
        ComponentFactory& m_componentFactory;

        bool m_running = true;
    public:
        Application(DX11::Device& device, DX11::Window& window, DX11::RenderModule& renderer, ResourceFactory& factory, ComponentFactory& componentFactory);
        Application(const Application& other) = delete;
        Application(Application&& other) noexcept = delete;

        Application& operator=(const Application& other) = delete;
        Application& operator=(Application&& other) noexcept = delete;


    public:
        int Run();
        void Quit();

    public:
        DX11::Window& GetWindow() const { return m_window; }
    };

    extern int RunApplication();
}