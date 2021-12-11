#pragma once

#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "Insanity_Math.h"
#include "Wrappers/ComponentWrapper.h"
#include "RendererComponent.h"

#include <memory>

namespace InsanityEngine::DX11
{
    class Renderer;
}

namespace InsanityEngine::DX11::StaticMesh
{
    struct Instance
    {
        ResourceHandle<Mesh> mesh;
        ResourceHandle<Material> material;

        Math::Types::Vector3f position;
        Math::Types::Vector3f scale;
        Math::Types::Quaternion<float> rotation;
    };


};

namespace InsanityEngine
{
    template<>
    struct Component<DX11::StaticMesh::Instance>
    {
        DX11::ComPtr<ID3D11Buffer> constantBuffer;
        DX11::StaticMesh::Instance data;
    };


    template<class HandleTy>
    class DX11::RendererComponentInterface<DX11::StaticMesh::Instance, HandleTy>
    {
    private:
        using ComponentType = Component<DX11::StaticMesh::Instance>;
        using HandleType = ComponentHandle<HandleTy>;

    public:
        void SetPosition(Math::Types::Vector3f position)
        {
            Get().data.position = position;
        }
        void SetRotation(Math::Types::Quaternion<float> rotation)
        {
            Get().data.rotation = rotation;
        }
        void SetScale(Math::Types::Vector3f scale)
        {
            Get().data.scale = scale;
        }

        Math::Types::Vector3f GetPosition() const { return Get().data.position; }
        Math::Types::Quaternion<float> GetRotation() const { return Get().data.rotation; }
        Math::Types::Vector3f GetScale() const { return Get().data.scale; }



    protected:
        ComponentType& Get()
        {
            return ToHandle().GetComponent();
        }
        const ComponentType& Get() const
        {
            return ToHandle().GetComponent();
        }

        auto& GetRenderer() { ToHandle().GetRenderer(); }
        const auto& GetRenderer() const { ToHandle().GetRenderer(); }

    private:
        HandleType& ToHandle() { return static_cast<HandleType&>(*this); }
        const HandleType& ToHandle() const { return static_cast<const HandleType&>(*this); }
    };

    
    template<>
    class ComponentHandle<DX11::StaticMesh::Instance> final : 
        public DX11::RendererComponentHandle<DX11::StaticMesh::Instance, DX11::Renderer>, 
        public DX11::RendererComponentInterface<DX11::StaticMesh::Instance, DX11::StaticMesh::Instance>
    {
        template<class ComponentTy, class HandleTy>
        friend class DX11::RendererComponentInterface;

    public:
        using DX11::RendererComponentHandle<DX11::StaticMesh::Instance, DX11::Renderer>::RendererComponentHandle;
    };
}