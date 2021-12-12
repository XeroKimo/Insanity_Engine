#pragma once
#include "../CommonInclude.h"
#include "Wrappers/ResourceWrapper.h"
#include <memory>

namespace InsanityEngine::DX11
{
    struct Mesh
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;

        UINT vertexCount = 0;
        UINT indexCount = 0;
    };
}


namespace InsanityEngine
{
    template<>
    struct Resource<DX11::Mesh> 
    {
        DX11::Mesh resource;
    };

    template<>
    class ResourceHandle<DX11::Mesh> : public SharedResourceHandle<DX11::Mesh>
    {
    public:
        using SharedResourceHandle::SharedResourceHandle;


    public:
        //ID3D11Buffer& GetVertexBuffer() const { return *GetUnderlyingResource()->resource.vertexBuffer.Get(); }
        //ID3D11Buffer& GetIndexBuffer() const { return  *GetUnderlyingResource()->resource.indexBuffer.Get(); }
        UINT GetVertexCount() const { return GetUnderlyingResource()->resource.vertexCount; }
        UINT GetIndexCount() const  { return GetUnderlyingResource()->resource.indexCount; }
    };
}