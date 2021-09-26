#include "MatrixExtension.h"

namespace InsanityEngine::Math::Functions::Matrix
{
    Types::Matrix4x4f PositionMatrix(Types::Vector3f position)
    {
        return
        {
            1, 0, 0, position.x(),
            0, 1, 0, position.y(),
            0, 0, 1, position.z(),
            0, 0, 0, 1
        };
    }


    Types::Matrix4x4f ScaleMatrix(Types::Vector3f scale)
    {
        return
        {
            scale.x(), 0, 0, 0,
            0, scale.y(), 0, 0,
            0, 0, scale.z(), 0,
            0, 0, 0, 1
        };
    }

    Types::Matrix4x4f RotateXMatrix(Types::Radians<float> angle)
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

    Types::Matrix4x4f RotateYMatrix(Types::Radians<float> angle)
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

    Types::Matrix4x4f RotateZMatrix(Types::Radians<float> angle)
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

    Types::Matrix4x4f RotateXMatrix(Types::Degrees<float> angle)
    {
        return RotateXMatrix(angle.ToRadians());
    }

    Types::Matrix4x4f RotateYMatrix(Types::Degrees<float> angle)
    {
        return RotateYMatrix(angle.ToRadians());
    }

    Types::Matrix4x4f RotateZMatrix(Types::Degrees<float> angle)
    {
        return RotateZMatrix(angle.ToRadians());
    }

    Types::Matrix4x4f PerspectiveProjectionLH(Types::Radians<float> fovY, float aspectRatio, float zNear, float zFar)
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

    Types::Matrix4x4f PerspectiveProjectionRH(Types::Radians<float> fovY, float aspectRatio, float zNear, float zFar)
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

    Types::Matrix4x4f PerspectiveProjectionLH(Types::Degrees<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        return PerspectiveProjectionLH(fovY.ToRadians(), aspectRatio, zNear, zFar);
    }

    Types::Matrix4x4f PerspectiveProjectionRH(Types::Degrees<float> fovY, float aspectRatio, float zNear, float zFar)
    {
        return PerspectiveProjectionRH(fovY.ToRadians(), aspectRatio, zNear, zFar);
    }

    Types::Matrix4x4f OrthographicProjectionLH(Types::Vector2f resolution, float zNear, float zFar)
    {
        return Types::Matrix4x4f
        {
            2 / resolution.x(), 0,      0,                              0,
            0,      2 / resolution.y(), 0,                              0,
            0,      0,     1 / (zFar - zNear),          -zNear / (zFar - zNear),
            0,      0,      0 , 1
        };
    }

    Types::Matrix4x4f OrthographicProjectionRH(Types::Vector2f resolution, float zNear, float zFar)
    {
        return Types::Matrix4x4f
        {
            2 / resolution.x(), 0,      0,                              0,
            0,      2 / resolution.y(), 0,                              0,
            0,      0,     1 / (zNear - zFar),          zNear / (zNear - zFar),
            0,      0,      0 , 1
        };
    }
}