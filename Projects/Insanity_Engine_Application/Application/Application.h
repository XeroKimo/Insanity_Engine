#pragma once
#include <memory>

namespace InsanityEngine::DX11
{
    class Device;
    class Window;
    class Renderer;
}

namespace InsanityEngine::Application
{

    class Application
    {
    private:
        DX11::Device& m_device;
        DX11::Window& m_window;
        DX11::Renderer& m_renderer;

        bool m_running = true;
    public:
        Application(DX11::Device& device, DX11::Window& window, DX11::Renderer& renderer);
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