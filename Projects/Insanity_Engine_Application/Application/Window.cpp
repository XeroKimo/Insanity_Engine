#include "Window.h"
#include "../DX11/Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "SDL_config_windows.h"
#include "SDL.h"
#include "SDL_syswm.h"

using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::Application
{
    Window::Window(std::string_view windowName, Vector2f windowSize, DX11::Device& device) :
        m_device(&device),
        m_handle(SDL_CreateWindow(windowName.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN), &SDL_DestroyWindow)
    {
        InitializeWindow(windowName, windowSize);
        InitializeSwapChain();
        InitializeBackBuffer();
    }

    Window::~Window()
    {
        m_swapChain->SetFullscreenState(false, nullptr);

    }

    void Window::Present()
    {
        m_swapChain->Present(1, 0);
    }

    Vector2f Window::GetWindowSize() const
    {
        int width;
        int height;
        SDL_GetWindowSize(m_handle.get(), &width, &height);
        return Vector2f(width, height);
    }

    void Window::InitializeWindow(std::string_view windowName, Math::Types::Vector2f windowSize)
    {
        //m_handle = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>();
    }

    void Window::InitializeSwapChain()
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
        ComPtr<ID3D11Device> device;

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(m_handle.get(), &info);

        if(HRESULT hr = factory->CreateSwapChainForHwnd(
            m_device->GetDevice(),
            info.info.win.window,
            &swapChainDesc,
            &fullscreenDesc,
            nullptr,
            tempSwapChain.GetAddressOf());
            FAILED(hr))
        {
            throw HRESULTException("Failed to create swap chain. HRESULT: ", hr);
        }

        if(HRESULT hr = tempSwapChain.As(&m_swapChain); FAILED(hr))
        {
            throw HRESULTException("Failed to query swap chain. HRESULT: ", hr);
        }
    }

    void Window::InitializeBackBuffer()
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if(FAILED(hr))
            throw HRESULTException("Failed to get back buffer. HRESULT: ", hr);

        D3D11_RENDER_TARGET_VIEW_DESC1 rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        hr = m_device->GetDevice()->CreateRenderTargetView1(backBuffer.Get(), &rtvDesc, &m_backBuffer);

        if(FAILED(hr))
            throw HRESULTException("Failed to create back buffer. HRESULT: ", hr);
    }
}