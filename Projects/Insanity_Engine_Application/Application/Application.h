#pragma once
#include <memory>

namespace InsanityEngine::DX11
{
    class Device;
}

namespace InsanityEngine::Application
{
    class Window;

    class Application
    {
    private:
        DX11::Device& m_device;
        Window& m_window;

        bool m_running = true;
    public:
        Application(DX11::Device& device, Window& window);
        Application(const Application& other) = delete;
        Application(Application&& other) noexcept = delete;

        Application& operator=(const Application& other) = delete;
        Application& operator=(Application&& other) noexcept = delete;


    public:
        int Run();
        void Quit();

    public:
        Window& GetWindow() const { return m_window; }
    };

    extern void RunApplication();
}