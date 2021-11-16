#pragma once
#include "../CommonInclude.h"
#include "../Internal/Resources.h"
#include "../../Factories/ResourceFactory.h"
#include "../InputLayouts.h"
#include <variant>
#include <span>

namespace InsanityEngine
{
    template<>
    struct ResourceInitializer<DX11::Mesh> : public ResourceInitializer<UnknownResource>
    {
        template<class VertexType>
        struct RawInit
        {
            std::span<VertexType> vertices;
            std::span<UINT> indices;
        };

        std::variant<RawInit<DX11::InputLayouts::PositionNormalUV::VertexData>> data;
    };


    template<>
    class Resource<DX11::Mesh> : public UnknownResource
    {
    private:
        DX11::Mesh m_mesh;

    public:
        Resource(std::string_view name, DX11::Mesh mesh);

    public:
        DX11::ComPtr<ID3D11Buffer> GetVertexBuffer() const { return m_mesh.vertexBuffer; }
        DX11::ComPtr<ID3D11Buffer> GetIndexBuffer() const { return m_mesh.indexBuffer; }

        UINT GetVertexCount() const { return m_mesh.vertexCount; }
        UINT GetIndexCount() const { return m_mesh.indexCount; }
    };
}