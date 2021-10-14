#include "Camera.h"
#include "Extensions/MatrixExtension.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include <assert.h> 

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::Engine
{
    Camera::Camera(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil) :
        m_renderTargetView(std::move(renderTarget)),
        m_depthStencilView(std::move(depthStencil))
    {
        assert(m_renderTargetView != nullptr);
    }

    void Camera::SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil)
    {
        assert(renderTarget != nullptr);

        m_renderTargetView = std::move(renderTarget);
        m_depthStencilView = std::move(depthStencil);
    }
    Math::Types::Matrix4x4f Camera::GetViewMatrix() const
    {
        return Math::Matrix::PositionMatrix(position);
    }
}