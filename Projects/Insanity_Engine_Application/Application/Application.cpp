#include "Application.h"
#include "Vector.h"

#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../Test Scenes/TriangleRender.h"
#include "Renderer.h"

#include "SDL.h"
#include <chrono>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::Application
{
    Application::Application(DX11::Device& device, Window& window, Renderer& renderer) :
        m_device(device),
        m_window(window),
        m_renderer(renderer)
    {
    }

    int Application::Run()
    {
        
        //TriangleRenderSetup(m_device, m_window);

        std::chrono::time_point previous = std::chrono::steady_clock::now();
        std::chrono::time_point now = previous;
        SDL_Event event;
        while(m_running)
        {
            if(SDL_PollEvent(&event))
            {
                //TriangleRenderInput(event);

                if(event.type == SDL_EventType::SDL_QUIT)
                    m_running = false;

               
            }
            else
            {
                previous = std::exchange(now, std::chrono::steady_clock::now());
                float delta = std::chrono::duration<float>(now - previous).count();


                m_window.Draw();
            }
        }

        ShutdownTriangleRender();

        return 0;
    }

    void Application::Quit()
    {
        m_running = false;
    }


    int RunApplication()
    {
        int retVal = 0;
        SDL_Init(SDL_INIT_VIDEO);

        try
        {
            DX11::Device device;
            Renderer renderer{ device };
            Window window{ "Insanity Engine", { 1280.f, 720.f }, device, renderer };


            Application app(device, window, renderer);
            app.Run();
        }
        catch(std::exception e)
        {
            retVal = 1;
        }

        SDL_Quit();

        return retVal;
    }
}
