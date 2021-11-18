#include "StaticMeshInstance.h"
#include "Extensions/MatrixExtension.h"
#include "../Renderer/Renderer.h"
#include <assert.h>


namespace InsanityEngine::DX11::StaticMesh
{
    //MeshObjectData::MeshObjectData(std::shared_ptr<Resource<Mesh>> mesh, std::shared_ptr<Resource<StaticMesh::Material>> material) :
    //    m_mesh(std::move(mesh)),
    //    m_material(std::move(material))
    //{
    //    assert(m_mesh != nullptr);
    //    assert(m_material != nullptr);
    //}

    //void MeshObjectData::SetMesh(std::shared_ptr<Resource<Mesh>> mesh)
    //{
    //    m_mesh = std::move(mesh);
    //    assert(m_mesh != nullptr);
    //}

    //void MeshObjectData::SetMaterial(std::shared_ptr<Resource<StaticMesh::Material>> material)
    //{
    //    m_material = std::move(material);
    //    assert(m_material != nullptr);
    //}

    //Math::Types::Matrix4x4f MeshObjectData::GetObjectMatrix() const
    //{
    //    return Math::Matrix::ScaleRotateTransformMatrix(Math::Matrix::ScaleMatrix(scale), rotation.ToRotationMatrix(), Math::Matrix::PositionMatrix(position));
    //}

    //MeshObject::MeshObject(ComPtr<ID3D11Buffer> constantBuffer, DX11::StaticMesh::MeshObjectData&& data) :
    //    m_objectConstants(constantBuffer),
    //    data(std::move(data))
    //{
    //    assert(m_objectConstants != nullptr);
    //}

}
namespace InsanityEngine
{
    void Component<DX11::StaticMesh::Instance>::SetPosition(Math::Types::Vector3f position)
    {
        Object().position = position;
    }

    void Component<InsanityEngine::DX11::StaticMesh::Instance>::SetRotation(Math::Types::Quaternion<float> rotation)
    {
        Object().rotation = rotation;
    }
    void Component<InsanityEngine::DX11::StaticMesh::Instance>::SetScale(Math::Types::Vector3f scale)
    {
        Object().scale = scale;
    }
    void Component<InsanityEngine::DX11::StaticMesh::Instance>::Translate(Math::Types::Vector3f position)
    {
        SetPosition(GetPosition() + position);
    }
    void Component<InsanityEngine::DX11::StaticMesh::Instance>::Rotate(Math::Types::Quaternion<float> rotation)
    {
        SetRotation(GetRotation() * rotation);
    }
    void Component<InsanityEngine::DX11::StaticMesh::Instance>::Scale(Math::Types::Vector3f scale)
    {
        SetScale(GetScale() * scale);
    }

    void Component<InsanityEngine::DX11::StaticMesh::Instance>::SetMaterial(ResourceHandle<InsanityEngine::DX11::StaticMesh::Material> material)
    {
        //Renderer().
    }

}