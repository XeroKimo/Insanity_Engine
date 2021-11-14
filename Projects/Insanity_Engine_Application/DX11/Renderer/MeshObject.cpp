#include "MeshObject.h"
#include "Extensions/MatrixExtension.h"
#include <assert.h>


namespace InsanityEngine::DX11::StaticMesh
{
    MeshObjectData::MeshObjectData(std::shared_ptr<Resource<Resources::Mesh>> mesh, std::shared_ptr<Resource<Resources::StaticMesh::Material>> material) :
        m_mesh(std::move(mesh)),
        m_material(std::move(material))
    {
        assert(m_mesh != nullptr);
        assert(m_material != nullptr);
    }

    void MeshObjectData::SetMesh(std::shared_ptr<Resource<Resources::Mesh>> mesh)
    {
        m_mesh = std::move(mesh);
        assert(m_mesh != nullptr);
    }

    void MeshObjectData::SetMaterial(std::shared_ptr<Resource<Resources::StaticMesh::Material>> material)
    {
        m_material = std::move(material);
        assert(m_material != nullptr);
    }

    Math::Types::Matrix4x4f MeshObjectData::GetObjectMatrix() const
    {
        return Math::Matrix::ScaleRotateTransformMatrix(Math::Matrix::ScaleMatrix(scale), rotation.ToRotationMatrix(), Math::Matrix::PositionMatrix(position));
    }

    MeshObject::MeshObject(ComPtr<ID3D11Buffer> constantBuffer, DX11::StaticMesh::MeshObjectData&& data) :
        m_objectConstants(constantBuffer),
        data(std::move(data))
    {
        assert(m_objectConstants != nullptr);
    }

    void MeshHandle::SetPosition(Math::Types::Vector3f position)
    {
        Object().data.position = position;
    }

    void MeshHandle::SetRotation(Math::Types::Quaternion<float> rotation)
    {
        Object().data.rotation = rotation;
    }

    void MeshHandle::SetScale(Math::Types::Vector3f scale)
    {
        Object().data.scale = scale;
    }

    void MeshHandle::Translate(Math::Types::Vector3f position)
    {
        SetPosition(GetPosition() + position);
    }

    void MeshHandle::Rotate(Math::Types::Quaternion<float> rotation)
    {
        SetRotation(GetRotation() * rotation);
    }

    void MeshHandle::Scale(Math::Types::Vector3f scale)
    {
        SetScale(GetScale() * scale);
    }

}