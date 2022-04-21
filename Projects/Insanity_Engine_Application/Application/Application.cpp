#include "Application.h"
#include "../Rendering/Window.h"
#include <dxgi1_6.h>
#include <utility>

namespace InsanityEngine::Application
{
    int Run(const Settings& settings)
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<ID3D12DebugDevice2> debugDevice;
        {
            Rendering::Window window = [&]() 
            {
                ComPtr<IDXGIFactory7> factory = TypedD3D::Helpers::DXGI::Factory::Create<IDXGIFactory7>(TypedD3D::Helpers::DXGI::Factory::CreationFlags::None).GetValue();
                ComPtr<ID3D12Debug3> debug = TypedD3D::Helpers::D3D12::GetDebugInterface<ID3D12Debug3>().GetValue();
                debug->EnableDebugLayer();

                if(settings.renderAPI == Rendering::RenderAPI::DX12)
                {
                    TypedD3D::D3D12::Device5 device = TypedD3D::D3D12::CreateDevice<TypedD3D::D3D12::Device5>(D3D_FEATURE_LEVEL_12_0, nullptr).GetValue();
                    debugDevice = TypedD3D::Helpers::COM::Cast<ID3D12DebugDevice2>(device.GetComPtr());

                    return Rendering::Window::Create<Rendering::D3D12::DefaultDraw>(
                        settings.applicationName,
                        { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
                        settings.windowResolution,
                        SDL_WINDOW_SHOWN,
                        *factory.Get(),
                        device);
                }
                else
                {
                    Microsoft::WRL::ComPtr<ID3D11Device> tempDevice;
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> tempDeviceContext;
                    D3D_FEATURE_LEVEL levels = D3D_FEATURE_LEVEL_11_1;
                    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &levels, 1, D3D11_SDK_VERSION, &tempDevice, nullptr, &tempDeviceContext);

                    Microsoft::WRL::ComPtr<ID3D11Device5> device = TypedD3D::Helpers::COM::Cast<ID3D11Device5>(tempDevice);
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext = TypedD3D::Helpers::COM::Cast<ID3D11DeviceContext4>(tempDeviceContext);

                    return Rendering::Window::Create<Rendering::D3D11::DefaultDraw>(
                        settings.applicationName,
                        { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
                        settings.windowResolution,
                        SDL_WINDOW_SHOWN,
                        *factory.Get(),
                        device,
                        deviceContext);
                }

            }();
            Rendering::D3D12::DefaultDraw* draw = window.GetRenderer<Rendering::D3D12::DefaultDraw>();
            SDL_Event event;
            while(true)
            {
                if(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_EventType::SDL_QUIT)
                        break;

                    window.HandleEvent(event);

                    switch(event.type)
                    {
                    case SDL_EventType::SDL_KEYDOWN:
                        if(event.key.repeat == 0 && event.key.state == SDL_PRESSED)
                        {
                            if(event.key.keysym.sym == SDL_KeyCode::SDLK_1)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                window.SetFullscreen(false);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_2)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                window.SetFullscreen(true);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_3)
                            {
                                if(!window.IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.SetWindowSize({ 1280, 720 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.SetWindowSize({ 1280, 720 });
                                }
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_4)
                            {
                                if(!window.IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.SetWindowSize({ 1600, 900 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.SetWindowSize({ 1600, 900 });
                                }
                            }
                        }
                    break;
                    }
                }
                else
                {   
                    window.Draw();
                }
            }
        }

        if(debugDevice)
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);

        return 0;
    }
}
