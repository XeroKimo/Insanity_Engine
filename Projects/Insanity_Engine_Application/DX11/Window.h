#pragma once

#include "CommonInclude.h"
#include "Insanity_Math.h"
#include "Debug Classes/Exceptions/HRESULTException.h"


#include "SDL_config_windows.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <memory>

namespace InsanityEngine::DX11
{
    class Device;

    template<class RendererTy>
    class Window
    {
    private:
        std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> m_handle;
        RendererTy m_renderer;


    public:
        Window(Device& device, std::string_view windowName, Math::Types::Vector2f windowSize) :
            m_handle(InitializeWindow(windowName, windowSize)),
            m_renderer(device, CreateSwapChain(&device.GetDevice()))
        {

        }

    public:
        RendererTy& GetRenderer() { return m_renderer; }

    private:
        std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> InitializeWindow(std::string_view windowName, Math::Types::Vector2i windowSize)
        {
            return std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
                    SDL_CreateWindow(
                        windowName.data(),
                        SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED,
                        windowSize.x(),
                        windowSize.y(),
                        SDL_WINDOW_SHOWN
                    ),
                    &SDL_DestroyWindow
                );
        }

        ComPtr<IDXGISwapChain1> CreateSwapChain(ID3D11Device5* device)
        {
            ComPtr<IDXGIFactory2> factory;

            HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = 0;
            swapChainDesc.Height = 0;
            swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
            fullscreenDesc.RefreshRate.Denominator = fullscreenDesc.RefreshRate.Numerator = 0;
            fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            fullscreenDesc.Windowed = true;

            ComPtr<IDXGISwapChain1> tempSwapChain;

            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            SDL_GetWindowWMInfo(m_handle.get(), &info);

            if(HRESULT hr = factory->CreateSwapChainForHwnd(
                device,
                info.info.win.window,
                &swapChainDesc,
                &fullscreenDesc,
                nullptr,
                tempSwapChain.GetAddressOf());
                FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create swap chain. HRESULT: ", hr);
            }

            return tempSwapChain;
        }
    };
}