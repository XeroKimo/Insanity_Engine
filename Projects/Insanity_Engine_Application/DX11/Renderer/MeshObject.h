#pragma once
//#include "../Resources.h"
#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "../Resources/Shader.h"
#include "../Resources/Texture.h"
#include "../../Factories/ResourceFactory.h"
#include "../../Factories/ComponentFactory.h"
#include "Handle.h"

namespace InsanityEngine::DX11::StaticMesh
{
    class Renderer;

    class MeshObjectData
    {
    public:
        //static std::shared_ptr<Mesh> defaultMesh;
        //static std::shared_ptr<Material> defaultMaterial;

    private:
        std::shared_ptr<Resource<Mesh>> m_mesh;
        std::shared_ptr<Resource<StaticMesh::Material>> m_material;

    public:
        Math::Types::Vector3f position;
        Math::Types::Vector3f scale{ Math::Types::Scalar(1.f) };
        Math::Types::Quaternion<float> rotation;

    public:
        MeshObjectData(std::shared_ptr<Resource<Mesh>> mesh, std::shared_ptr<Resource<StaticMesh::Material>> material);

    public:
        void SetMesh(std::shared_ptr<Resource<Mesh>> mesh);
        void SetMaterial(std::shared_ptr<Resource<StaticMesh::Material>> material);

    public:
        std::shared_ptr<Resource<Mesh>> GetMesh() const { return m_mesh; }
        std::shared_ptr<Resource<StaticMesh::Material>> GetMaterial() const { return m_material; }

    public:
        Math::Types::Matrix4x4f GetObjectMatrix() const;
    };


    class MeshObject
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    private:
        ComPtr<ID3D11Buffer> m_objectConstants;

    public:
        DX11::StaticMesh::MeshObjectData data;

    public:
        MeshObject(ComPtr<ID3D11Buffer> constantBuffer, DX11::StaticMesh::MeshObjectData&& data);

    public:
        ID3D11Buffer* GetConstantBuffer() const { return m_objectConstants.Get(); }
    };

}

template<>
struct InsanityEngine::ComponentInitializer<InsanityEngine::DX11::StaticMesh::MeshObject>
{
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::Mesh> mesh;
    InsanityEngine::ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> material;

};

template<>
class InsanityEngine::Component<InsanityEngine::DX11::StaticMesh::MeshObject> : public InsanityEngine::DX11::ManagedHandle<InsanityEngine::DX11::StaticMesh::MeshObject, InsanityEngine::DX11::StaticMesh::Renderer>
{
private:
    using Base = InsanityEngine::DX11::ManagedHandle<InsanityEngine::DX11::StaticMesh::MeshObject, InsanityEngine::DX11::StaticMesh::Renderer>;

public:
    using Base::ManagedHandle;


public:
    void SetPosition(Math::Types::Vector3f position);
    void SetRotation(Math::Types::Quaternion<float> rotation);
    void SetScale(Math::Types::Vector3f scale);

    void Translate(Math::Types::Vector3f position);
    void Rotate(Math::Types::Quaternion<float> rotation);
    void Scale(Math::Types::Vector3f scale);

    Math::Types::Vector3f GetPosition() const { return Object().data.position; }
    Math::Types::Quaternion<float> GetRotation() const { return Object().data.rotation; }
    Math::Types::Vector3f GetScale() const { return Object().data.scale; }

    ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> GetMaterial() { return Object().data.GetMaterial(); }
};