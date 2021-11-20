#pragma once
#include "../CommonInclude.h"
#include "Insanity_Math.h"
#include "../Internal/Handle.h"
#include "../../Factories/ComponentFactory.h"
#include "../Resources/Texture.h"

namespace InsanityEngine::DX11
{
    class RenderModule;
    namespace StaticMesh
    {
        class Renderer;
    }

    struct ClipPlane
    {
        float Near = 0.0003f; 
        float Far = 1000.f;
    };

    struct Camera
    {
        ComPtr<ID3D11Buffer> constantBuffer;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11DepthStencilView> depthStencilView;
        ComPtr<ID3D11DepthStencilState> depthStencilState;

        Math::Types::Vector3f position;
        Math::Types::Quaternion<float> rotation;

        float fov = 90;
        ClipPlane clipPlane;

    public:
        Math::Types::Matrix4x4f GetViewMatrix() const;
        Math::Types::Matrix4x4f GetPerspectiveMatrix() const;
    };
}

template<>
struct InsanityEngine::ComponentInitializer<InsanityEngine::DX11::Camera> 
{
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::Texture> optionalTexture;
};

template<>
class InsanityEngine::Component<InsanityEngine::DX11::Camera> : public InsanityEngine::DX11::Handle<InsanityEngine::DX11::Camera, InsanityEngine::DX11::RenderModule>
{
    using Base = InsanityEngine::DX11::Handle<InsanityEngine::DX11::Camera, InsanityEngine::DX11::RenderModule>;
public:
    using Base::Handle;

public:
    void SetPosition(Math::Types::Vector3f position);
    void SetRotation(Math::Types::Quaternion<float> rotation);

    void Translate(Math::Types::Vector3f position);
    void Rotate(Math::Types::Quaternion<float> rotation);

    void SetClipPlane(InsanityEngine::DX11::ClipPlane plane);

    Math::Types::Vector3f GetPosition() const { return Object().position; }
    Math::Types::Quaternion<float> GetRotation() const { return Object().rotation; }
    InsanityEngine::DX11::ClipPlane GetClipPlane() const { return Object().clipPlane; }
};