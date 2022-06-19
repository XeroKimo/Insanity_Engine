#pragma once
#include "Insanity_Math.h"

namespace InsanityEngine::Rendering::Common::VertexFormat
{
    namespace Position
    {
        struct Format
        {
            Math::Types::Vector3f position;
        };
    }

    namespace PositionNormalUV
    {
        struct Format
        {
            Math::Types::Vector3f position;
            Math::Types::Vector3f normal;
            Math::Types::Vector2f uv;
        };
    }
}