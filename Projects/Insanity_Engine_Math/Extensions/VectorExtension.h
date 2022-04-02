#pragma once
#include "../Vector.h"
#include "../Trigonometry.h"

namespace InsanityEngine::Math::Vector
{
    Types::Vector3f ScreenToWorldPosition(Types::Vector2f position, Types::Vector2f screenResolution, const Types::Matrix4x4f& viewMatrix, const Types::Matrix4x4f& projectionMatrix, float depthMin, float depthMax, float depth);
}