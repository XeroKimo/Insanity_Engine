#include "Material.h"
#include <assert.h>

namespace InsanityEngine
{
    using namespace DX11;
    Resource<DX11::StaticMesh::Material>::Resource(std::string_view name, DX11::StaticMesh::Material material) :
        UnknownResource(name),
        m_material(material)
    {

    }

    void Resource<StaticMesh::Material>::SetColor(Math::Types::Vector4f color)
    {
        m_material.color = color;
    }
}