#include "Application.h"
#include "../Rendering/Window.h"
#include "../Rendering/Renderer.h"
#include <dxgi1_6.h>

namespace InsanityEngine::Application
{
    int Run(const Settings& settings)
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<ID3D12DebugDevice2> debugDevice;
        {
            ComPtr<IDXGIFactory7> factory = TypedD3D::Helpers::DXGI::Factory::Create<IDXGIFactory7>(TypedD3D::Helpers::DXGI::Factory::CreationFlags::None).GetValue();
            ComPtr<ID3D12Debug3> debug = TypedD3D::Helpers::D3D12::GetDebugInterface<ID3D12Debug3>().GetValue();
            debug->EnableDebugLayer();
            TypedD3D::D3D12::Device5 device = TypedD3D::D3D12::CreateDevice<TypedD3D::D3D12::Device5>(D3D_FEATURE_LEVEL_12_0, nullptr).GetValue();
            debugDevice = TypedD3D::Helpers::COM::Cast<ID3D12DebugDevice2>(device.GetComPtr());
            Rendering::D3D12::DefaultDraw defaultDraw;

            Rendering::Window<Rendering::PolymorphicRenderer> window(
                settings.applicationName,
                { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
                settings.windowResolution,
                SDL_WINDOW_SHOWN,
                *factory.Get(),
                device,
                defaultDraw);

            SDL_Event event;
            while(true)
            {
                if(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_EventType::SDL_QUIT)
                        break;
                    else if(event.type == SDL_WINDOWEVENT)
                    {
                        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            window.GetRenderer().ResizeBuffers({ event.window.data1, event.window.data2 });
                        }
                    }
                    else if(event.type == SDL_EventType::SDL_KEYDOWN)
                    {
                        if(event.key.repeat == 0 && event.key.state == SDL_PRESSED)
                        {
                            if(event.key.keysym.sym == SDL_KeyCode::SDLK_1)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                window.GetRenderer().SetFullscreen(false);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_2)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                window.GetRenderer().SetFullscreen(true);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_3)
                            {
                                if(!window.GetRenderer().IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.GetRenderer().SetWindowSize({ 1280, 720 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.GetRenderer().SetWindowSize({ 1280, 720 });
                                }
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_4)
                            {
                                if(!window.GetRenderer().IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.GetRenderer().SetWindowSize({ 1600, 900 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.GetRenderer().SetWindowSize({ 1600, 900 });
                                }
                            }
                        }
                    }
                }
                else
                {
                    window.GetRenderer().Draw();
                }
            }
        }
        debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);

        return 0;
    }
}
