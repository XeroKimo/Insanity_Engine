#include "Application.h"
#include "Vector.h"

#include "../DX11/Window.h"
#include "../DX11/Device.h"
#include "../DX11/Helpers.h"
#include "../DX11/ShaderConstants.h"
#include "../DX11/Resources.h"
#include "../Test Scenes/TriangleRender.h"
#include "../Test Scenes/TriangleRenderScene2.h"
#include "../DX11/Renderer/Renderer.h"
#include "Extensions/MatrixExtension.h"
#include "../Factories/ResourceFactory.h"
#include "../DX11/RenderModule.h"
#include "../Factories/ComponentFactory.h"

#include "SDL.h"
#include <chrono>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;


namespace InsanityEngine::Application
{
    Application::Application(DX11::Device& device, DX11::Window& window, DX11::RenderModule& renderer, ResourceFactory& factory, ComponentFactory& componentFactory) :
        m_device(device),
        m_window(window),
        m_renderer(renderer),
        m_factory(factory),
        m_componentFactory(componentFactory)
    {

    }

    int Application::Run()
    {
        
        TriangleRenderSetup2(m_device, m_window, m_factory, m_componentFactory);
        //TriangleRenderSetup(m_device, m_window);

        std::chrono::time_point previous = std::chrono::steady_clock::now();
        std::chrono::time_point now = previous;
        SDL_Event event;

        m_window.clearColor = { 0, 0.3f, 0.7f, 1 };

        while(m_running)
        {
            if(SDL_PollEvent(&event))
            {
                TriangleRenderInput2(event);
                //TriangleRenderInput(event);

                if(event.type == SDL_EventType::SDL_QUIT)
                    m_running = false;

            }
            else
            {
                previous = std::exchange(now, std::chrono::steady_clock::now());
                float delta = std::chrono::duration<float>(now - previous).count();

                TriangleRenderUpdate2(delta);
                m_renderer.Update(delta);

                m_renderer.Draw();
                m_window.Present();
            }
        }

        TriangleRenderShutdown2();

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
            ResourceFactory resourceFactory;
            ComponentFactory componentFactory;

            DX11::Device device;
            DX11::Window window{ "Insanity Engine", { 1280.f, 720.f }, device };
            DX11::RenderModule renderModule{ resourceFactory, componentFactory, device, window };


            Application app(device, window, renderModule, resourceFactory, componentFactory);
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
