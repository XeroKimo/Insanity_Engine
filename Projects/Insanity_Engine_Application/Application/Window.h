#pragma once

#include "Vector.h"
#include <Windows.h>
#include <wrl/client.h>
#include <string_view>
#include <memory>

struct IDXGISwapChain4;
struct ID3D11RenderTargetView1;
struct SDL_Window;


namespace InsanityEngine::DX11
{
    class Device;
}

namespace InsanityEngine::Application
{
    class Window
    {
    private:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    private:
        static constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;

    private:

        DX11::Device* m_device = nullptr;
        ComPtr<IDXGISwapChain4> m_swapChain;
        ComPtr<ID3D11RenderTargetView1> m_backBuffer;

        std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> m_handle;
    public:
        Window(std::string_view windowName, Math::Types::Vector2f windowSize, DX11::Device& device);
        ~Window();

    public:
        void Present();

    public:
        ID3D11RenderTargetView1* GetBackBuffer() const { return m_backBuffer.Get(); }
        Math::Types::Vector2f GetWindowSize() const;


    private:
        void InitializeWindow(std::string_view windowName, Math::Types::Vector2f windowSize);
        void InitializeSwapChain();
        void InitializeBackBuffer();
    };


}