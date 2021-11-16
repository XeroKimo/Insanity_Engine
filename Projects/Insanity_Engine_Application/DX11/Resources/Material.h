#pragma once
#include "../CommonInclude.h"
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
        std::shared_ptr<Resource<DX11::Shader>> shader;
        std::shared_ptr<Resource<DX11::Texture>> albedo;
        Math::Types::Vector4f color{ Math::Types::Scalar(1) };
    };


    template<>
    class Resource<DX11::StaticMesh::Material> : public UnknownResource
    {
    private:
        DX11::StaticMesh::Material m_material;

    public:
        Resource(std::string_view name, DX11::StaticMesh::Material material);

    public:
        void SetColor(Math::Types::Vector4f color);

    public:
        DX11::ComPtr<ID3D11Buffer> GetConstantBuffer() const { return m_material.constantBuffer; }
        std::shared_ptr<Resource<DX11::Shader>> GetShader() const { return m_material.shader; }
        std::shared_ptr<Resource<DX11::Texture>> GetAlbedo() const { return m_material.albedo; }
        Math::Types::Vector4f GetColor() const { return m_material.color; }
    };
}