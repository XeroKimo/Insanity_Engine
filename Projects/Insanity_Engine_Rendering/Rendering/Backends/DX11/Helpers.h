#pragma once

#include "Common_Include.h"
#include "Insanity_Math.h"

namespace InsanityEngine::Helpers::Rendering::DX11
{
    Math::Types::Vector3ui GetTextureDimensions(ID3D11Resource& resource);
}