#include "Texture.h"

namespace InsanityEngine::Rendering
{
    using namespace Math::Types;
    Vector3ui Texture::GetSize() const
    {
        if(std::holds_alternative<DX11::Texture>(m_underlyingType))
        {
            return std::get<DX11::Texture>(m_underlyingType).GetSize();
        }
        return Vector3ui();
    }
}