#include "Mesh.h"
#include <assert.h>

namespace InsanityEngine
{
    using namespace DX11;
    Resource<Mesh>::Resource(std::string_view name, DX11::Mesh mesh) :
        UnknownResource{ name },
        m_mesh{ mesh }
    {
        assert(m_mesh.vertexBuffer != nullptr);
    }
}