#include "Application.h"
#include "Vector.h"

#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../Test Scenes/TriangleRender.h"

#include <chrono>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::Application
{
    Application::Application(DX11::Device& device, Window& window) :
        m_device(device),
        m_window(window)
    {
    }

    int Application::Run()
    {
        MSG msg; 
        
        TriangleRenderSetup(m_device, m_window);

        std::chrono::time_point previous = std::chrono::steady_clock::now();
        std::chrono::time_point now = previous;
        
        while(m_running)
        {
            if(m_window.PollEvent(msg))
            {
                if(msg.message == WM_QUIT)
                    Quit();
            }
            else
            {
                previous = now;
                now = std::chrono::steady_clock::now();
                std::chrono::duration<float> delta = now - previous;

                TriangleRenderUpdate(std::chrono::duration<float>(delta).count());
                TriangleRender(m_device, m_window);
                m_window.Present();
            }
        }

        ShutdownTriangleRender();

        return 0;
    }

    void Application::Quit()
    {
        m_running = false;
    }


    void RunApplication()
    { 
        DX11::Device device;
        Window window{ L"Insanity Engine", { 1280.f, 720.f }, device };


        Application app(device, window);
        app.Run();


    }
}
