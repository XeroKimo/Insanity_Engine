#include "Camera.h"
#include "Extensions/MatrixExtension.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "../Helpers.h"
#include <assert.h> 

using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine
{
    Math::Types::Matrix4x4f Camera::GetViewMatrix() const
    {
        return Math::Matrix::PositionMatrix(-position) * rotation.ToRotationMatrix();
    }
    Math::Types::Matrix4x4f Camera::GetPerspectiveMatrix() const
    {
        Vector2f resolution = Helpers::GetTextureResolution(*renderTargetView.Get());
        return Math::Matrix::PerspectiveProjectionLH(Degrees(fov), resolution.x() / resolution.y(), clipPlane.Near, clipPlane.Far);
    }

    void Component<Camera>::SetPosition(Vector3f position)
    {
        Object().position = position;
    }

    void Component<Camera>::SetRotation(Math::Types::Quaternion<float> rotation)
    {
        Object().rotation = rotation;
    }
        
    void Component<Camera>::Translate(Math::Types::Vector3f position)
    {
        SetPosition(GetPosition() + position);
    }
    void Component<Camera>::Rotate(Math::Types::Quaternion<float> rotation)
    {
        SetRotation(GetRotation() * rotation);
    }
         
    void Component<Camera>::SetClipPlane(InsanityEngine::DX11::ClipPlane plane)
    {
        Object().clipPlane = plane;
    }
}