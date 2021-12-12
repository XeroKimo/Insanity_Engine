#pragma once

#include "RendererComponent.h"
#include "Insanity_Math.h"
#include "../CommonInclude.h"
#include "Wrappers/ComponentWrapper.h"

namespace InsanityEngine::DX11
{
    class Renderer;

    struct ClipPlane
    {
        float Near = 0.0003f;
        float Far = 1000.f;
    };

    struct Camera
    {
        Math::Types::Vector3f position;
        Math::Types::Quaternion<float> rotation;
        ClipPlane clipPlane;
        float fov = 90;
    };
}

namespace InsanityEngine
{
    template<>
    struct Component<DX11::Camera>
    {
        DX11::ComPtr<ID3D11RenderTargetView> renderTargetView;
        DX11::ComPtr<ID3D11DepthStencilView> depthStencilView;
        //ComPtr<ID3D11DepthStencilState> depthStencilState;
        DX11::ComPtr<ID3D11Buffer> constantBuffer;
        DX11::Camera data;


    public:
        Math::Types::Matrix4x4f GetViewMatrix() const;
        Math::Types::Matrix4x4f GetPerspectiveMatrix() const;
    };
}