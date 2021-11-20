#include "StaticMeshInstance.h"
#include "Extensions/MatrixExtension.h"
#include "../Renderer/Renderer.h"
#include <assert.h>


namespace InsanityEngine
{
    using namespace DX11;
    using namespace Math::Types;
    void Component<StaticMesh::Instance>::SetPosition(Vector3f position)
    {
        Object().position = position;
    }

    void Component<StaticMesh::Instance>::SetRotation(Quaternion<float> rotation)
    {
        Object().rotation = rotation;
    }
    void Component<StaticMesh::Instance>::SetScale(Vector3f scale)
    {
        Object().scale = scale;
    }
    void Component<StaticMesh::Instance>::Translate(Vector3f position)
    {
        SetPosition(GetPosition() + position);
    }
    void Component<StaticMesh::Instance>::Rotate(Quaternion<float> rotation)
    {
        SetRotation(GetRotation() * rotation);
    }
    void Component<StaticMesh::Instance>::Scale(Vector3f scale)
    {
        SetScale(GetScale() * scale);
    }

    void Component<StaticMesh::Instance>::SetMaterial(ResourceHandle<StaticMesh::Material> material)
    {
        if(material == nullptr)
        {
        }
        else
        {
            Object().material = material.GetResourcePointer();
        }
    }

}