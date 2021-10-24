#pragma once
#include "../CommonInclude.h"
#include "Vector.h"

namespace InsanityEngine::DX11
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
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    public:
        Math::Types::Vector3f position;
        float fov = 90;
        ClipPlane clipPlane;
        
    public:
        Camera(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil = nullptr, ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr);

    public:
        void SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil = nullptr);
        void SetDepthStencilState(ComPtr<ID3D11DepthStencilState> depthStencilState);

    public:
        ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const { return m_depthStencilView.Get(); }
        ID3D11DepthStencilState* GetDepthStencilState() const { return m_depthStencilState.Get(); }

        Math::Types::Matrix4x4f GetViewMatrix() const;
        Math::Types::Matrix4x4f GetPerspectiveMatrix() const;

        Math::Types::Vector2f GetRenderTargetResolution() const;
    };

}