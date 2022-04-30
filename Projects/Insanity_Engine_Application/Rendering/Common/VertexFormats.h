#pragma once
#include "Insanity_Math.h"

namespace InsanityEngine::Rendering::Common::VertexFormat
{
    struct Position
    {
        Math::Types::Vector3f position;
    };

    struct PositionUV
    {
        Math::Types::Vector3f position;
        Math::Types::Vector2f uv;
    };
}