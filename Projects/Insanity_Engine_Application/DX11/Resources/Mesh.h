#pragma once
#include "../CommonInclude.h"
#include "../Internal/Resources.h"
#include "../../Factories/ResourceFactory.h"
#include "../InputLayouts.h"
#include <variant>
#include <span>

namespace InsanityEngine
{
    namespace DX11::StaticMesh
    {
        class Renderer;
    }

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
    class ResourceHandle<DX11::Mesh> : public UserDefinedResourceHandle<DX11::Mesh>
    {
        using Base = UserDefinedResourceHandle<DX11::Mesh>;
        friend class DX11::StaticMesh::Renderer;

        template<class T>
        friend class Component;
    public:
        using Base::UserDefinedResourceHandle;
    };
}