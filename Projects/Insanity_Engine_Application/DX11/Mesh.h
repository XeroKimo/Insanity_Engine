#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"
#include <array>

namespace InsanityEngine::DX11::StaticMesh
{
    using namespace Math::Types;

    struct VertexData
    {
        Vector3f position;
        Vector3f normal;
        Vector2f uv;
    };



    class Mesh
    {
    private:
        ComPtr<ID3D11Buffer> m_vertexBuffer;
        ComPtr<ID3D11Buffer> m_indexBuffer;

        UINT m_vertexCount = 0;
        UINT m_indexCount = 0;

    public:
        Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount);

    public:
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }

        UINT GetVertexCount() const { return m_vertexCount; }
        UINT GetIndexCount() const { return m_indexCount; }

    public:
        friend bool operator==(const Mesh& lh, const Mesh& rh) = default;
        friend bool operator!=(const Mesh& lh, const Mesh& rh) = default;

    };



    class MeshObject
    {
    public:
        Mesh mesh;
        Vector3f position;

    public:
        MeshObject(Mesh mesh);

    public:
        Matrix4x4f GetObjectMatrix() const;
    };

    extern std::array<D3D11_INPUT_ELEMENT_DESC, 3> GetInputElementDescription();

}