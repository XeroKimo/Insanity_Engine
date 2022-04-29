#include "dx11.h"
#include "../Window.h"
#include <d3dcompiler.h>

namespace InsanityEngine::Rendering::D3D11
{
    static constexpr UINT bufferCount = 2;

    Backend::Backend(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext) :
        m_device(device),
        m_deviceContext(deviceContext),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory,
            *m_device.Get(),
            std::any_cast<HWND>(window.GetWindowHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            bufferCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue())
    {
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    Backend::~Backend()
    {
        m_deviceContext->Flush();
        SetFullscreen(false);
    }

    void Backend::ResizeBuffers(Math::Types::Vector2ui size)
    {
        m_deviceContext->Flush();

        m_backBuffer->Release();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->ResizeBuffers(bufferCount, size.x(), size.y(), desc.Format, desc.Flags);
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    void Backend::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        m_deviceContext->Flush();

        m_backBuffer->Release();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(bufferCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    void Backend::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = swapChainDesc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(&modeDesc);
    }

    void Backend::Present()
    {
        m_swapChain->Present(1, 0);
    }

    struct Vertex
    {
        float x;
        float y;
        float z;
    };

    DefaultDraw::DefaultDraw(Backend& renderer) :
        m_renderer(&renderer)
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<ID3DBlob> vertexBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(L"Default_Resources/VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexBlob, &errorBlob);
        if(FAILED(hr))
        {
            char* message = static_cast<char*>(errorBlob->GetBufferPointer());
            std::unique_ptr<wchar_t[]> messageT = std::make_unique<wchar_t[]>(errorBlob->GetBufferSize());
            MultiByteToWideChar(CP_ACP, MB_COMPOSITE, message, static_cast<int>(errorBlob->GetBufferSize()), messageT.get(), static_cast<int>(errorBlob->GetBufferSize()));
            //MessageBox(handle, messageT.get(), L"Error", MB_OK);
            return;
        }
        m_renderer->GetDevice()->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &m_vertexShader);

        ComPtr<ID3DBlob> pixelBlob;
        hr = D3DCompileFromFile(L"Default_Resources/PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBlob, nullptr);
        if(FAILED(hr))
            return;

        m_renderer->GetDevice()->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &m_pixelShader);

        std::array<D3D11_INPUT_ELEMENT_DESC, 1> inputLayout
        {
            D3D11_INPUT_ELEMENT_DESC
            {
                .SemanticName = "POSITION",
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .InputSlot = 0,
                .AlignedByteOffset = 0,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            }
        };

        m_renderer->GetDevice()->CreateInputLayout(inputLayout.data(), static_cast<UINT>(inputLayout.size()), vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &m_inputLayout);

        auto vertices = std::to_array<Vertex>(
            {
                { -0.5f, -0.5f, 0 },
                {  0.0f,  0.5f, 0 },
                {  0.5f, -0.5f, 0 },
            });

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = vertices.data();
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;

        m_renderer->GetDevice()->CreateBuffer(&bufferDesc, &data, &m_vertexBuffer);
    }

    void DefaultDraw::Draw()
    {
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_renderer->GetWindowSize().x());
        viewport.Height = static_cast<float>(m_renderer->GetWindowSize().y());
        viewport.MaxDepth = 1;
        viewport.MinDepth = 0;

        D3D11_RECT rect = {};
        rect.right = m_renderer->GetWindowSize().x();
        rect.bottom = m_renderer->GetWindowSize().y();

        m_renderer->GetDeviceContext()->ClearRenderTargetView(&m_renderer->GetBackBufferResource(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }).data());
        ID3D11RenderTargetView* targets[] = { &m_renderer->GetBackBufferResource() };
        m_renderer->GetDeviceContext()->OMSetRenderTargets(1, targets, nullptr);
        m_renderer->GetDeviceContext()->RSSetViewports(1, &viewport);
        m_renderer->GetDeviceContext()->RSSetScissorRects(1, &rect);
        m_renderer->GetDeviceContext()->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_renderer->GetDeviceContext()->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_renderer->GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
        m_renderer->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        m_renderer->GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        m_renderer->GetDeviceContext()->Draw(3, 0);

        m_renderer->Present();
    }
}