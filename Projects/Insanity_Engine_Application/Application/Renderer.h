#pragma once
#include "../Application/Window.h"


namespace InsanityEngine::Application
{
    class Renderer : public BaseRenderer
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        DX11::Device* m_device = nullptr;

    public:
        Renderer(DX11::Device& device);

    protected:
        void Draw(ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D11RenderTargetView1> backBuffer) override;
    };
}