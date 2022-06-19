#include "Application.h"
#include "../Rendering/Window.h"
#include "source/Helpers/DXGIHelpers.h"
#include "source/Helpers/D3D12Helpers.h"
#include "source/Helpers/COMHelpers.h"
#include "DX11/Backend.h"
#include <dxgi1_6.h>
#include <utility>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace InsanityEngine::Application
{
    int Run(const Settings& settings)
    {
        using WindowHandle = std::unique_ptr < SDL_Window, decltype([](SDL_Window* w) { SDL_DestroyWindow(w); }) > ;

        WindowHandle window(SDL_CreateWindow("Insanity Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN));

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(window.get(), &info);
        D3D_FEATURE_LEVEL levels = D3D_FEATURE_LEVEL_11_0;
        auto [device, deviceContext] = TypedD3D::D3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, std::span(&levels, 1), D3D11_SDK_VERSION).value();
        Rendering::D3D11::BackendInitParams initParams
        {
            .device = device,
            .deviceContext = deviceContext,
            .factory = TypedD3D::DXGI::Factory::Create1<IDXGIFactory2>().value(),
            .windowHandle = info.info.win.window,
            .windowSize = { 1280, 720 }
        };
        Rendering::D3D11::BackendWithRenderer<Rendering::D3D11::DefaultDraw> renderer{ initParams };
        //using Microsoft::WRL::ComPtr;
        //ComPtr<ID3D12DebugDevice2> debugDevice;
        //{
        //    Rendering::Window window = [&]() 
        //    {
        //        ComPtr<IDXGIFactory7> factory = TypedD3D::Helpers::DXGI::Factory::Create<IDXGIFactory7>(TypedD3D::Helpers::DXGI::Factory::CreationFlags::None).value();
        //        ComPtr<ID3D12Debug3> debug = TypedD3D::Helpers::D3D12::GetDebugInterface<ID3D12Debug3>().value();
        //        debug->EnableDebugLayer();

        //        if(settings.renderAPI == Rendering::RenderAPI::DX12)
        //        {
        //            TypedD3D::Wrapper<ID3D12Device5> device = TypedD3D::D3D12::CreateDevice<TypedD3D::D3D12::Device5>(D3D_FEATURE_LEVEL_12_0, nullptr).value();
        //            debugDevice = TypedD3D::Helpers::COM::Cast<ID3D12DebugDevice2>(device.AsComPtr());

        //            return Rendering::Window::Create<Rendering::D3D12::DefaultDraw>(
        //                settings.applicationName,
        //                { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
        //                settings.windowResolution,
        //                SDL_WINDOW_SHOWN,
        //                *factory.Get(),
        //                device);
        //        }
        //        else
        //        {
        //            Microsoft::WRL::ComPtr<ID3D11Device> tempDevice;
        //            Microsoft::WRL::ComPtr<ID3D11DeviceContext> tempDeviceContext;
        //            D3D_FEATURE_LEVEL levels = D3D_FEATURE_LEVEL_11_1;
        //            D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &levels, 1, D3D11_SDK_VERSION, &tempDevice, nullptr, &tempDeviceContext);

        //            Microsoft::WRL::ComPtr<ID3D11Device5> device = TypedD3D::Helpers::COM::Cast<ID3D11Device5>(tempDevice);
        //            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext = TypedD3D::Helpers::COM::Cast<ID3D11DeviceContext4>(tempDeviceContext);

        //            return Rendering::Window::Create<Rendering::D3D11::DefaultDraw>(
        //                settings.applicationName,
        //                { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
        //                settings.windowResolution,
        //                SDL_WINDOW_SHOWN,
        //                *factory.Get(),
        //                device,
        //                deviceContext);
        //        }

        //    }();

        //    Rendering::D3D12::DefaultDraw* draw = window.GetRenderer<Rendering::D3D12::DefaultDraw>();
        SDL_Event event;
        while(true)
        {
            if(SDL_PollEvent(&event))
            {
                if(event.type == SDL_EventType::SDL_QUIT)
                    break;

                //window.HandleEvent(event);
            }
            else
            {
                renderer.Draw();
            }
        }

    //if(debugDevice)
    //    debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);

        return 0;
    }
}
