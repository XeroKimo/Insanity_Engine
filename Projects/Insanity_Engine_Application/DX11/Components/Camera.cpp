#include "Camera.h"
#include "Extensions/MatrixExtension.h"
#include "../Helpers.h"

namespace InsanityEngine
{
    using namespace Math::Types;

    Math::Types::Matrix4x4f Component<DX11::Camera>::GetViewMatrix() const
    {
        return (-data.rotation).ToRotationMatrix() * Math::Matrix::PositionMatrix(-data.position);
    }

    Math::Types::Matrix4x4f Component<DX11::Camera>::GetPerspectiveMatrix() const
    {
        Vector2f resolution = DX11::Helpers::GetTextureResolution(*renderTargetView.Get());
        return Math::Matrix::PerspectiveProjectionLH(Degrees(data.fov), resolution.x() / resolution.y(), data.clipPlane.Near, data.clipPlane.Far);

    }
}