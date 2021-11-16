#include "Texture.h"
#include <assert.h>

namespace InsanityEngine
{
    using namespace DX11;


    Resource<Texture>::Resource(std::string_view name, Texture texture) :
        UnknownResource{ name },
        m_texture{ texture }
    {
        assert(texture.shaderResource != nullptr);
        assert(texture.sampler != nullptr);
    }
}