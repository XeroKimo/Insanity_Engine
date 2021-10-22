#include "Renderer.h"
#include "../DX11/Device.h"
#include "../DX11/Helpers.h"
#include "../DX11/CommonInclude.h"

using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::Application
{
    Renderer::Renderer(DX11::Device& device) :
        m_device(&device)
    {

    }

    void Renderer::Draw(ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D11RenderTargetView1> backBuffer)
    {
        DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), backBuffer.Get(), Vector4f { 0, 0.3f, 0.7f, 1 });
    }
}