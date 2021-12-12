#pragma once

#include "../CommonInclude.h"
#include "Texture.h"
#include "Shader.h"
#include "Wrappers/ResourceWrapper.h"
#include "Insanity_Math.h"

namespace InsanityEngine::DX11::StaticMesh
{
    struct Material
    {
        ResourceHandle<Texture> texture;
        ResourceHandle<Shader> shader;
        Math::Types::Vector4f color;
    };

}

namespace InsanityEngine
{
    template<>
    struct Resource<DX11::StaticMesh::Material>
    {
        DX11::StaticMesh::Material resource;
        DX11::ComPtr<ID3D11Buffer> constantBuffer;
    };

    template<>
    class ResourceHandle<DX11::StaticMesh::Material> : public SharedResourceHandle<DX11::StaticMesh::Material>
    {
    public:
        using SharedResourceHandle::SharedResourceHandle;

    public:
        ResourceHandle<DX11::Texture> GetTexture() const { return GetUnderlyingResource()->resource.texture; }
        ResourceHandle<DX11::Shader> GetShader() const { return GetUnderlyingResource()->resource.shader; }
        Math::Types::Vector4f GetColor() const { return GetUnderlyingResource()->resource.color;        }
    };
}