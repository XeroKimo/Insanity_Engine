#pragma once
#include "Insanity_Math.h"
#include "../Rendering/Device.h"
#include <string>

namespace InsanityEngine::Application
{
    struct Settings
    {
        std::string applicationName;
        Math::Types::Vector2i windowResolution;
        Rendering::RenderAPI renderAPI;
    };
}