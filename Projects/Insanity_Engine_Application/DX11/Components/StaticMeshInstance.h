#pragma once
//#include "../Resources.h"
#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "../Resources/Shader.h"
#include "../Resources/Texture.h"
#include "../../Factories/ResourceFactory.h"
#include "../../Factories/ComponentFactory.h"
#include "../Internal/Handle.h"

namespace InsanityEngine::DX11::StaticMesh
{
    class Renderer;

    struct Instance
    {
        std::shared_ptr<Resource<Mesh>> mesh;
        std::shared_ptr<Resource<Material>> material;
        ComPtr<ID3D11Buffer> constantBuffer;

        Math::Types::Vector3f position;
        Math::Types::Vector3f scale{ Math::Types::Scalar(1.f) };
        Math::Types::Quaternion<float> rotation;
    };
}

template<>
struct InsanityEngine::ComponentInitializer<InsanityEngine::DX11::StaticMesh::Instance>
{
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::Mesh> mesh;
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> material;

};

template<>
class InsanityEngine::Component<InsanityEngine::DX11::StaticMesh::Instance> : public InsanityEngine::DX11::Handle<InsanityEngine::DX11::StaticMesh::Instance, InsanityEngine::DX11::StaticMesh::Renderer>
{
private:
    using Base = InsanityEngine::DX11::Handle<InsanityEngine::DX11::StaticMesh::Instance, InsanityEngine::DX11::StaticMesh::Renderer>;

public:
    using Base::Handle;


public:
    void SetPosition(Math::Types::Vector3f position);
    void SetRotation(Math::Types::Quaternion<float> rotation);
    void SetScale(Math::Types::Vector3f scale);

    void Translate(Math::Types::Vector3f position);
    void Rotate(Math::Types::Quaternion<float> rotation);
    void Scale(Math::Types::Vector3f scale);

    Math::Types::Vector3f GetPosition() const { return Object().position; }
    Math::Types::Quaternion<float> GetRotation() const { return Object().rotation; }
    Math::Types::Vector3f GetScale() const { return Object().scale; }

    void SetMaterial(ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> material);
    ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> GetMaterial() { return Object().material; }
};