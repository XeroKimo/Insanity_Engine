#pragma once
#include "Backends/DX11/DX11Texture.h"
#include <variant>

namespace InsanityEngine::Rendering
{
    class Texture
    {
    private:
        std::variant<DX11::Texture> m_underlyingType;


    public:
        Math::Types::Vector3ui GetTextureDimensions() const;

        const std::variant<DX11::Texture>& GetUnderlyingType() const { return m_underlyingType; }
    };

}