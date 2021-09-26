#pragma once
#include "../DX11/CommonInclude.h"
#include "Vector.h"

namespace InsanityEngine::Engine
{
    struct ClipPlane
    {
        float Near = 1000.f;
        float Far = 0.0003f;
    };

    class Camera
    {
    private:
        template<class T>
        using ComPtr = DX11::ComPtr<T>;


    private:
        ComPtr<ID3D11Device5> m_device;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    public:
        Math::Types::Vector3f position;
        float fov = 90;
        ClipPlane clipPlane;
        
    public:
        Camera(ComPtr<ID3D11Device5> device, ComPtr<ID3D11Texture2D> renderTargetTexture, bool createDepthBuffer);
        Camera(ComPtr<ID3D11Device5> device, ComPtr<ID3D11RenderTargetView> renderTargetView, bool createDepthBuffer);

    public:
        void SetRenderTarget(ComPtr<ID3D11Texture2D> renderTargetTexture);
        void SetRenderTarget(ComPtr<ID3D11RenderTargetView> renderTargetView);

        void CreateDepthBuffer();
        void DestroyDepthBuffer();

    public:
        ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const { return m_depthStencilView.Get(); }

        Math::Types::Matrix4x4f GetViewMatrix() const;

    private:
        void CreateRenderTargetView(ComPtr<ID3D11Texture2D> renderTargetTexture);
        void CreateDepthStencilView(ComPtr<ID3D11Texture2D> renderTargetTexture, float resolutionMultiplier);
    };

}