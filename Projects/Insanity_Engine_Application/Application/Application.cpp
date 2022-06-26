#include "Application.h"
#include "../Rendering/Window.h"
#include "source/Helpers/DXGIHelpers.h"
#include "source/Helpers/D3D12Helpers.h"
#include "source/Helpers/COMHelpers.h"
#include "Window.h"
#include "DX11/Backend.h"
#include "DX12/Backend.h"
#include <dxgi1_6.h>
#include <utility>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace InsanityEngine::Application
{
    int Run(const Settings& settings)
    {
        TypedD3D::Wrapper<ID3D12Device5> device = TypedD3D::D3D12::CreateDevice<ID3D12Device5>(D3D_FEATURE_LEVEL_11_0).value();

        Rendering::Window<Rendering::D3D12::BackendWithRenderer<Rendering::D3D12::DefaultDraw>> window
        { 
            "Insanity Engine", 
            {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED}, 
            {1280, 720}, 
            SDL_WINDOW_SHOWN, 
            device, 
            TypedD3D::DXGI::Factory::Create1<IDXGIFactory2>().value()
        };

        SDL_Event event;
        while(true)
        {
            if(SDL_PollEvent(&event))
            {
                if(event.type == SDL_EventType::SDL_QUIT)
                    break;

                if(event.type == SDL_EventType::SDL_KEYDOWN)
                {
                    switch(event.key.keysym.sym)
                    {
                    case SDL_KeyCode::SDLK_1:

                        SDL_SetWindowResizable(window.GetHandle(), SDL_TRUE);
                        window.SetWindowSize({ 1600, 900 });
                        SDL_SetWindowResizable(window.GetHandle(), SDL_FALSE);
                        break;
                    case SDL_KeyCode::SDLK_2:
                        SDL_SetWindowResizable(window.GetHandle(), SDL_TRUE);
                        window.SetWindowSize({ 1280, 720 });
                        SDL_SetWindowResizable(window.GetHandle(), SDL_FALSE);
                        break;
                    }
                }
                if(event.type == SDL_EventType::SDL_WINDOWEVENT)
                {
                    if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        window.ResizeBuffers({ event.window.data1, event.window.data2 });
                }
            }
            else
            {
                window.Draw();
            }
        }

    //if(debugDevice)
    //    debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);

        return 0;
    }
}
