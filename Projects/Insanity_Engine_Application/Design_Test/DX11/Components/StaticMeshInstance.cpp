#include "StaticMeshInstance.h"

namespace InsanityEngine
{
    using namespace DX11;
    using namespace DX11::StaticMesh;
    using namespace Math::Types;

    //void ComponentHandleInterface<StaticMesh::Instance>::SetPosition(Vector3f position)
    //{
    //    Get().component.position = position;
    //}

    //void ComponentHandleInterface<DX11::StaticMesh::Instance>::SetRotation(Math::Types::Quaternion<float> rotation)
    //{
    //    Get().component.rotation = rotation;
    //}

    //void ComponentHandleInterface<DX11::StaticMesh::Instance>::SetScale(Math::Types::Vector3f scale)
    //{
    //    Get().component.scale = scale;
    //}

    //Component<StaticMesh::Instance>& ComponentHandleInterface<StaticMesh::Instance>::Get()
    //{
    //    return static_cast<HandleTy&>(*this).GetComponent();
    //}

    //const Component<StaticMesh::Instance>& ComponentHandleInterface<StaticMesh::Instance>::Get() const
    //{
    //    return static_cast<const HandleTy&>(*this).GetComponent();
    //}

    //auto& ComponentHandleInterface<DX11::StaticMesh::Instance>::GetRenderer()
    //{
    //    using base = RendererComponent<StaticMesh::Instance, Renderer>;

    //    return static_cast<HandleTy&>(*this).base::GetRenderer();
    //}

    //const auto& ComponentHandleInterface<DX11::StaticMesh::Instance>::GetRenderer() const
    //{
    //    using base = RendererComponent<StaticMesh::Instance, Renderer>;

    //    return static_cast<const HandleTy&>(*this).base::GetRenderer();
    //}
}