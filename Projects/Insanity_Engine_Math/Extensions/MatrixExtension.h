#pragma once
#include "../Matrix.h"
#include "../Trignometry.h"

namespace InsanityEngine::Math::Functions::Matrix
{
    extern Types::Matrix4x4f PositionMatrix(Types::Vector3f position);
    extern Types::Matrix4x4f ScaleMatrix(Types::Vector3f scale);

    extern Types::Matrix4x4f RotateXMatrix(Types::Radians<float> angle);
    extern Types::Matrix4x4f RotateYMatrix(Types::Radians<float> angle);
    extern Types::Matrix4x4f RotateZMatrix(Types::Radians<float> angle);

    extern Types::Matrix4x4f RotateXMatrix(Types::Degrees<float> angle);
    extern Types::Matrix4x4f RotateYMatrix(Types::Degrees<float> angle);
    extern Types::Matrix4x4f RotateZMatrix(Types::Degrees<float> angle);

    extern Types::Matrix4x4f PerspectiveProjectionLH(Types::Radians<float> fovY, float aspectRatio, float zNear, float zFar);
    extern Types::Matrix4x4f PerspectiveProjectionRH(Types::Radians<float> fovY, float aspectRatio, float zNear, float zFar);

    extern Types::Matrix4x4f PerspectiveProjectionLH(Types::Degrees<float> fovY, float aspectRatio, float zNear, float zFar);
    extern Types::Matrix4x4f PerspectiveProjectionRH(Types::Degrees<float> fovY, float aspectRatio, float zNear, float zFar);

    extern Types::Matrix4x4f OrthographicProjectionLH(Types::Vector2f resolution, float zNear, float zFar);
    extern Types::Matrix4x4f OrthographicProjectionRH(Types::Vector2f resolution, float zNear, float zFar);
}