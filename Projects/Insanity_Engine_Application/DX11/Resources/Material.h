#pragma once
#include "../CommonInclude.h"
#include "Shader.h"
#include "Texture.h"
#include "../../Factories/ResourceFactory.h"
#include "Insanity_Math.h"
#include <memory>

namespace InsanityEngine
{
    namespace DX11
    {
        struct Texture;
        struct Shader;


        namespace StaticMesh
        {
            class Renderer;
            struct Material
            {
                DX11::ComPtr<ID3D11Buffer> constantBuffer;
                std::shared_ptr<Resource<Shader>> shader;
                std::shared_ptr<Resource<Texture>> albedo;
                Math::Types::Vector4f color;
            };
        }
    }

    template<>
    struct ResourceInitializer<DX11::StaticMesh::Material> : public ResourceInitializer<UnknownResource>
    {
        ResourceHandle<DX11::Shader> shader;
        ResourceHandle<DX11::Texture> albedo;
        Math::Types::Vector4f color{ Math::Types::Scalar(1) };
    };


    template<>
    class ResourceHandle<DX11::StaticMesh::Material> : public UserDefinedResourceHandle<DX11::StaticMesh::Material>
    {
        using Base = UserDefinedResourceHandle<DX11::StaticMesh::Material>;
        friend class DX11::StaticMesh::Renderer;

        template<class T>
        friend class Component;
    public:
        using Base::UserDefinedResourceHandle;

    public:
        void SetColor(Math::Types::Vector4f color);

        std::shared_ptr<Resource<DX11::Shader>> GetShader() const { return GetResource().Get().shader; }
        std::shared_ptr<Resource<DX11::Texture>> GetAlbedo() const { return GetResource().Get().albedo; }
        Math::Types::Vector4f GetColor() const { return GetResource().Get().color; }

    };
}