#include "Window.h"
#include "../DX11/Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"

using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::Application
{
    Window::Window(std::wstring_view windowName, Vector2f windowSize, DX11::Device& device) :
        m_device(&device)
    {
        InitializeWindow(windowName, windowSize);
        InitializeSwapChain();
        InitializeBackBuffer();


    }

    Window::~Window()
    {
        m_swapChain->SetFullscreenState(false, nullptr);
    }


    bool Window::PollEvent(MSG& msg)
    {
        if(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            return true;
        }

        return false;
    }

    void Window::Present()
    {
        m_swapChain->Present(1, 0);
    }

    Vector2f Window::GetWindowSize() const
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        return Vector2f(rect.right, rect.bottom);
    }

    void Window::InitializeWindow(std::wstring_view windowName, Math::Types::Vector2f windowSize)
    {
        WNDCLASSW windowClass{};
        windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        windowClass.hInstance = NULL;
        windowClass.lpfnWndProc = WndProc;
        windowClass.lpszClassName = windowName.data();
        windowClass.style = CS_HREDRAW | CS_VREDRAW;

        if(!RegisterClass(&windowClass))
        {
            MessageBox(NULL, L"Could not register class", L"Error", MB_OK);
            throw std::exception("Could not register class");
        }

        RECT rect = {};
        rect.bottom = static_cast<LONG>(windowSize.y());
        rect.right = static_cast<LONG>(windowSize.x());

        AdjustWindowRect(&rect, windowStyle, false);

        m_hwnd = CreateWindowExW(0,
            windowName.data(),
            windowName.data(),
            windowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            NULL,
            NULL);


        if(!m_hwnd)
        {
            MessageBox(NULL, L"Could not create window", L"Error", MB_OK);
            throw std::exception("Could not create window");
        }

        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        ShowWindow(m_hwnd, SW_RESTORE);
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

        if(HRESULT hr =
            factory->CreateSwapChainForHwnd(m_device->GetDevice(),
                m_hwnd,
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
    LRESULT Window::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch(uMsg)
        {
        case WM_KEYDOWN:
            //BlocksEngine::Debug::LogInfo("WndProc: Key pressed: {}", wParam);
            return 0;
        case WM_SIZE:
            //if(window)
            //{
            //    window->ResizeBuffers({ LOWORD(lParam), HIWORD(lParam) });
            //}
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }

        return 0;
    }
}