#include "Backend.h"
#include "VertexFormats.h"
#include <d3dcompiler.h>

namespace InsanityEngine::Rendering::D3D11
{
    Backend::Backend(const BackendInitParams& params) :
        m_device(params.device),
        m_deviceContext(params.deviceContext),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain3>(
            *params.factory.Get(),
            *m_device.get().Get(),
            params.windowHandle,
            params.swapChainFormat,
            params.bufferCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).value()),
        m_backBuffer(CreateBackBuffer())
    {
    }

    Backend::~Backend()
    {
        m_deviceContext->Flush();
        SetFullscreen(false);
    }

    void Backend::ResizeBuffers(Math::Types::Vector2ui size)
    {
        m_deviceContext->Flush();

        m_backBuffer = nullptr;

        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();
        m_swapChain->ResizeBuffers(desc.BufferCount, size.x(), size.y(), desc.Format, desc.Flags);
        m_backBuffer = CreateBackBuffer();
    }

    void Backend::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        m_deviceContext->Flush();

        m_backBuffer = nullptr;

        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(desc.BufferCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        m_backBuffer = CreateBackBuffer();
    }

    void Backend::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = desc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(modeDesc);
    }

    void Backend::Present()
    {
        m_swapChain->Present(1, 0);
    }

    TypedD3D::Wrapper<ID3D11RenderTargetView> Backend::CreateBackBuffer()
    {
        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();

        D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Format = desc.Format;

        if(viewDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
            viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        else if(viewDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
            viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        
        return m_device->CreateRenderTargetView(m_swapChain->GetBuffer<ID3D11Resource>(0).value(), &viewDesc).value();
    }

    using Vertex = Common::VertexFormat::Position::Format;

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
        m_vertexShader = m_renderer->GetDevice()->CreateVertexShader(*vertexBlob.Get(), nullptr).value();

        ComPtr<ID3DBlob> pixelBlob;
        hr = D3DCompileFromFile(L"Default_Resources/PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBlob, nullptr);
        if(FAILED(hr))
            return;

        m_pixelShader = m_renderer->GetDevice()->CreatePixelShader(*pixelBlob.Get(), nullptr).value();

        m_inputLayout = VertexFormat::Position::CreateLayout(m_renderer->GetDevice(), vertexBlob).value();

        auto vertices = std::to_array<Vertex>(
            {
                {{ -0.5f, -0.5f, 0 }},
                {{  0.0f,  0.5f, 0 }},
                {{  0.5f, -0.5f, 0 }},
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

        m_vertexBuffer = m_renderer->GetDevice()->CreateBuffer(bufferDesc, &data).value();
    }

    void DefaultDraw::Draw(Backend& backend)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_renderer->GetWindowSize().x());
        viewport.Height = static_cast<float>(m_renderer->GetWindowSize().y());
        viewport.MaxDepth = 1;
        viewport.MinDepth = 0;

        D3D11_RECT rect = {};
        rect.right = m_renderer->GetWindowSize().x();
        rect.bottom = m_renderer->GetWindowSize().y();

        std::array clearColor = { 0.0f, 0.3f, 0.7f, 1.0f };
        m_renderer->GetDeviceContext()->ClearRenderTargetView(m_renderer->GetBackBufferResource(), std::span(clearColor));
        TypedD3D::Wrapper<ID3D11RenderTargetView> targets = m_renderer->GetBackBufferResource();
        m_renderer->GetDeviceContext()->OMSetRenderTargets(std::span(&targets, 1), {});
        m_renderer->GetDeviceContext()->RSSetViewports({ &viewport, 1 });
        m_renderer->GetDeviceContext()->RSSetScissorRects({ &rect, 1 });
        m_renderer->GetDeviceContext()->VSSetShader(m_vertexShader, {});
        m_renderer->GetDeviceContext()->PSSetShader(m_pixelShader, {});
        m_renderer->GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
        m_renderer->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        TypedD3D::Stride stride{ sizeof(Vertex) };
        TypedD3D::Offset offset{ 0 };

        m_renderer->GetDeviceContext()->IASetVertexBuffers(0, xk::span_tuple<TypedD3D::Wrapper<ID3D11Buffer>, std::dynamic_extent, const TypedD3D::Stride, const TypedD3D::Offset>(&m_vertexBuffer, 1, &stride, &offset));
        m_renderer->GetDeviceContext()->Draw(3, 0);

        m_renderer->Present();
    }
}