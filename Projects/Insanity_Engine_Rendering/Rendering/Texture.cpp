#include "Texture.h"

namespace InsanityEngine::Rendering
{
    using namespace Math::Types;
    Vector3ui Texture::GetTextureDimensions() const
    {
        if(std::holds_alternative<DX11::Texture>(m_underlyingType))
        {
            return std::get<DX11::Texture>(m_underlyingType).GetTextureDimensions();
        }
        return Vector3ui();
    }
}