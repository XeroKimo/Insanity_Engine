#include "MatrixExtension.h"

using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::Math::Matrix
{
    Matrix4x4f PositionMatrix(Vector3f position)
    {
        return
        {
            1, 0, 0, position.x(),
            0, 1, 0, position.y(),
            0, 0, 1, position.z(),
            0, 0, 0, 1
        };
    }


    Matrix4x4f ScaleMatrix(Vector3f scale)
    {
        return
        {
            scale.x(), 0, 0, 0,
            0, scale.y(), 0, 0,
            0, 0, scale.z(), 0,
            0, 0, 0, 1
        };
    }

    Matrix4x4f RotateXMatrix(Radians<float> angle)
    {
        float cosAngle = cos(angle.Data());
        float sinAngle = sin(angle.Data());

        return
        {
            1, 0,           0,          0,
            0, cosAngle,    -sinAngle,  0,
            0, sinAngle,    cosAngle,   0,
            0, 0,           0,          1
        };
    }

    Matrix4x4f RotateYMatrix(Radians<float> angle)
    {
        float cosAngle = cos(angle.Data());
        float sinAngle = sin(angle.Data());

        return
        {
            cosAngle,   0,      sinAngle,   0,
            0,          1,      0,          0,
            -sinAngle,  0,      cosAngle,   0,
            0,          0,      0,          1
        };
    }

    Matrix4x4f RotateZMatrix(Radians<float> angle)
    {
        float cosAngle = cos(angle.Data());
        float sinAngle = sin(angle.Data());

        return
        {
            cosAngle,   -sinAngle,  0,  0,
            sinAngle,   cosAngle,   0,  0,
            0,          0,          1,  0,
            0,          0,          0,  1
        };
    }

    Matrix4x4f RotateXMatrix(Degrees<float> angle)
    {
        return RotateXMatrix(angle.ToRadians());
    }

    Matrix4x4f RotateYMatrix(Degrees<float> angle)
    {
        return RotateYMatrix(angle.ToRadians());
    }

    Matrix4x4f RotateZMatrix(Degrees<float> angle)
    {
        return RotateZMatrix(angle.ToRadians());
    }

    Matrix4x4f PerspectiveProjectionLH(Radians<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        float yScale = 1 / tanf(fovY.Data() / 2);
        float xScale = yScale / aspectRatio;

        return Types::Matrix4x4f
        {
            xScale, 0,      0,                              0,
            0,      yScale, 0,                              0,
            0,      0,      zFar / (zFar - zNear),          -zNear * zFar / (zFar - zNear),
            0,      0,      1 , 0
        };
    }

    Matrix4x4f PerspectiveProjectionRH(Radians<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        float yScale = 1 / tanf(fovY.Data() / 2);
        float xScale = yScale / aspectRatio;

        return Types::Matrix4x4f
        {
            xScale, 0,      0,                              0,
            0,      yScale, 0,                              0,
            0,      0,      zFar / (zNear - zFar),          zNear * zFar / (zNear - zFar),
            0,      0,      -1 , 0
        };
    }

    Matrix4x4f PerspectiveProjectionLH(Degrees<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        return PerspectiveProjectionLH(fovY.ToRadians(), aspectRatio, zNear, zFar);
    }

    Matrix4x4f PerspectiveProjectionRH(Degrees<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        return PerspectiveProjectionRH(fovY.ToRadians(), aspectRatio, zNear, zFar);
    }

    Matrix4x4f OrthographicProjectionLH(Types::Vector2f resolution, float zNear, float zFar)
    {
        return Types::Matrix4x4f
        {
            2 / resolution.x(), 0,      0,                              0,
            0,      2 / resolution.y(), 0,                              0,
            0,      0,     1 / (zFar - zNear),          -zNear / (zFar - zNear),
            0,      0,      0 , 1
        };
    }

    Matrix4x4f OrthographicProjectionRH(Vector2f resolution, float zNear, float zFar)
    {
        return Types::Matrix4x4f
        {
            2 / resolution.x(), 0,      0,                              0,
            0,      2 / resolution.y(), 0,                              0,
            0,      0,     1 / (zNear - zFar),          zNear / (zNear - zFar),
            0,      0,      0 , 1
        };
    }

    Types::Matrix4x4f ViewProjectionMatrix(const Types::Matrix4x4f& view, const Types::Matrix4x4f& projection)
    {
        return projection * view;
    }

    Types::Matrix4x4f WorldViewProjectionMatrix(const Types::Matrix4x4f& world, const Types::Matrix4x4f& view, const Types::Matrix4x4f& projection)
    {
        return ViewProjectionMatrix(view, projection) * world;
    }

    Types::Matrix4x4f ScaleRotateTransformMatrix(const Types::Matrix4x4f& scale, const Types::Matrix4x4f& rotate, const Types::Matrix4x4f& transform)
    {
        return transform * rotate * scale;
    }
}