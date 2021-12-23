#pragma once
#include "Backends/DX11/DX11Texture.h"
#include <variant>

namespace InsanityEngine::Rendering
{
    class Texture
    {
        using underlying_t = std::variant<DX11::Texture>;
    private:
        underlying_t m_underlyingType;

    public:
        template<class T>
        Texture(T texture) : m_underlyingType(texture) 
        {
                
        }

    public:
        Math::Types::Vector3ui GetSize() const;

        template<class T>
        bool IsType() const { return std::holds_alternative<T>(m_underlyingType); }

        template<class T>
        T& GetUnderlyingType() { return std::get<T>(m_underlyingType); }

        template<class T>
        const T& GetUnderlyingType() const { return std::get<T>(m_underlyingType); }
    };

}