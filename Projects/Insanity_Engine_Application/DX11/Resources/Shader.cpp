#include "Shader.h"

namespace InsanityEngine
{
    using namespace DX11;
    Resource<Shader>::Resource(std::string_view name, Shader shader) :
        UnknownResource{ name },
        m_shader{ shader }
    {
    }

}