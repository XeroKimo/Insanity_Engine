#include "Camera.h"
#include "Extensions/MatrixExtension.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include <assert.h> 

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::DX11
{
    CameraData::CameraData(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil, ComPtr<ID3D11DepthStencilState> depthStencilState) :
        m_renderTargetView(std::move(renderTarget)),
        m_depthStencilView(std::move(depthStencil)),
        m_depthStencilState(std::move(depthStencilState))
    {
        assert(m_renderTargetView != nullptr);
    }

    void CameraData::SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil)
    {
        assert(renderTarget != nullptr);

        m_renderTargetView = std::move(renderTarget);
        m_depthStencilView = std::move(depthStencil);
    }
    void CameraData::SetDepthStencilState(ComPtr<ID3D11DepthStencilState> depthStencilState)
    {
        m_depthStencilState = std::move(depthStencilState);
    }
    Math::Types::Matrix4x4f CameraData::GetViewMatrix() const
    {
        return Math::Matrix::PositionMatrix(-position) * rotation.ToRotationMatrix();
    }
    Math::Types::Matrix4x4f CameraData::GetPerspectiveMatrix() const
    {
        Vector2f resolution = GetRenderTargetResolution();
        return Math::Matrix::PerspectiveProjectionLH(Degrees(fov), resolution.x() / resolution.y(), clipPlane.Near, clipPlane.Far);
    }
    Math::Types::Vector2f CameraData::GetRenderTargetResolution() const
    {
        ComPtr<ID3D11Resource> resource;
        m_renderTargetView->GetResource(&resource);

        ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);

        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);

        return Math::Types::Vector2f(desc.Width, desc.Height);
    }


    CameraObject::CameraObject(ComPtr<ID3D11Buffer> cameraConstants, DX11::CameraData&& data) :
        m_cameraConstants(cameraConstants),
        data(std::move(data))
    {
        assert(m_cameraConstants != nullptr);
    }

    //void CameraHandle::SetPosition(Math::Types::Vector3f position)
    //{
    //    Object().data.position = position;
    //}
    //void CameraHandle::SetRotation(Math::Types::Quaternion<float> rotation)
    //{
    //    Object().data.rotation = rotation;
    //}
}