#pragma once
#include "../CommonInclude.h"
#include "Insanity_Math.h"
#include "../Internal/Handle.h"
#include "../../Factories/ComponentFactory.h"
#include "../Resources/Texture.h"

namespace InsanityEngine::DX11
{
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
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        Math::Types::Vector3f position;
        Math::Types::Quaternion<float> rotation;

        float fov = 90;
        ClipPlane clipPlane;
    };

    class CameraData
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
        Math::Types::Quaternion<float> rotation;
        float fov = 90;
        ClipPlane clipPlane;
        
    public:
        CameraData(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil = nullptr, ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr);

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


    class CameraObject
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    private:
        ComPtr<ID3D11Buffer> m_cameraConstants;

    public:
        CameraData data;

    public:
        CameraObject(ComPtr<ID3D11Buffer> cameraConstants, CameraData&& camera);


    public:

        ID3D11Buffer* GetConstantBuffer() const { return m_cameraConstants.Get(); }
    };
}

template<>
struct InsanityEngine::ComponentInitializer<InsanityEngine::DX11::Camera> 
{
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::Texture> optionalTexture;
};

template<>
struct InsanityEngine::Component<InsanityEngine::DX11::Camera> : public InsanityEngine::DX11::Handle<InsanityEngine::DX11::Camera, InsanityEngine::DX11::StaticMesh::Renderer>
{

};