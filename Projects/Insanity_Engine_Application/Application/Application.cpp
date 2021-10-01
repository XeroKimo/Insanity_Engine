#include "Application.h"
#include "Vector.h"

#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../Test Scenes/TriangleRender.h"

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
        while(m_running)
        {
            if(m_window.PollEvent(msg))
            {
                if(msg.message == WM_QUIT)
                    Quit();
            }
            else
            {
                TriangleRenderUpdate(1.f / 144.f);
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
