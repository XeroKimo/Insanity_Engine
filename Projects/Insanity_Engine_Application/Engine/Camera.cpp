#include "Camera.h"
#include "Extensions/MatrixExtension.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include <assert.h> 

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::Engine
{
    Camera::Camera(ComPtr<ID3D11Device5> device, ComPtr<ID3D11Texture2D> renderTargetTexture, bool createDepthBuffer) :
        m_device(device)
    {
        assert(device != nullptr);

        SetRenderTarget(renderTargetTexture);

        if(createDepthBuffer)
            CreateDepthBuffer();
    }

    Camera::Camera(ComPtr<ID3D11Device5> device, ComPtr<ID3D11RenderTargetView> renderTargetView, bool createDepthBuffer) :
        m_device(device)
    {
        assert(device != nullptr);

        SetRenderTarget(renderTargetView);

        if(createDepthBuffer)
            CreateDepthBuffer();
    }

    void Camera::SetRenderTarget(ComPtr<ID3D11Texture2D> renderTargetTexture)
    {
        assert(renderTargetTexture != nullptr);

        CreateRenderTargetView(renderTargetTexture);
    }

    void Camera::SetRenderTarget(ComPtr<ID3D11RenderTargetView> renderTargetView)
    {
        assert(renderTargetView != nullptr);

        m_renderTargetView = renderTargetView;
    }

    void Camera::CreateDepthBuffer()
    {
        ComPtr<ID3D11Resource> resource;
        m_renderTargetView->GetResource(&resource);

        ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);

        CreateDepthStencilView(texture, 1.0f);
    }

    void Camera::DestroyDepthBuffer()
    {
        m_depthStencilView = nullptr;
    }

    Matrix4x4f Camera::GetViewMatrix() const
    {
        //return Math::Functions::Matrix::PositionMatrix(position);
        return Math::Matrix::PositionMatrix({ -position.x(), -position.y(), position.z() });
    }

    void Camera::CreateRenderTargetView(ComPtr<ID3D11Texture2D> renderTargetTexture)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        renderTargetTexture->GetDesc(&desc);

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = desc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;

        HRESULT hr = m_device->CreateRenderTargetView(renderTargetTexture.Get(), &rtvDesc, &m_renderTargetView);
        if(FAILED(hr))
        {
            throw HRESULTException("Failed to create render target view", hr);
        }
    }

    void Camera::CreateDepthStencilView(ComPtr<ID3D11Texture2D> renderTargetTexture, float resolutionMultiplier)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        renderTargetTexture->GetDesc(&desc);

        //desc.Height = static_cast<UINT>(static_cast<float>(desc.Height) * resolutionMultiplier);
        //desc.Width = static_cast<UINT>(static_cast<float>(desc.Width) * resolutionMultiplier);
        desc.MipLevels = 0;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11Texture2D> depthStencilTexture;
        HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &depthStencilTexture);

        if(FAILED(hr))
        {
            throw HRESULTException("Failed to create depth stencil texture", hr);
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = 0;
        depthStencilDesc.Texture2D.MipSlice = 0;

        hr = m_device->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &m_depthStencilView);

        if(FAILED(hr))
        {
            throw HRESULTException("Failed to create depth stencil view", hr);
        }
    }
}