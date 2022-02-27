#pragma once
#include "Insanity_Math.h"
#include <string>

namespace InsanityEngine::Application
{
    struct Settings
    {
        std::string applicationName;
        Math::Types::Vector2i windowResolution;
    };
}