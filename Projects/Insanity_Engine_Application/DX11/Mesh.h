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
        friend bool operator==(const Mesh& lh, const Mesh& rh) = default;
        friend bool operator!=(const Mesh & lh, const Mesh & rh) = default;

    public:
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }

        UINT GetVertexCount() const { return m_vertexCount; }
        UINT GetIndexCount() const { return m_indexCount; }

    };

    extern std::array<D3D11_INPUT_ELEMENT_DESC, 3> GetInputElementDescription();

    class Texture
    {
        ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        ComPtr<ID3D11SamplerState> m_samplerState;

    public:
        Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState);

    public:
        friend bool operator==(const Texture& lh, const Texture& rh) = default;
        friend bool operator!=(const Texture& lh, const Texture& rh) = default;

    public:
        void SetSamplerState(ComPtr<ID3D11SamplerState> samplerState);

        ID3D11ShaderResourceView* GetView() const { return m_shaderResourceView.Get(); }
        ID3D11SamplerState* GetSamplerState() const { return m_samplerState.Get(); }
    };

    class Material
    {
    private:
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;

    public:
        Texture albedo;
        Vector4f color;

    public:
        Material(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader, Texture albedo, Vector4f color = Vector4f(Scalar(1.f)));

    public:
        friend bool operator==(const Material& lh, const Material& rh) = default;
        friend bool operator!=(const Material& lh, const Material& rh) = default;

    public:
        ID3D11VertexShader* GetVertexShader() const { return m_vertexShader.Get(); }
        ID3D11PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }
    };


    class MeshObject
    {
    public:
        Mesh mesh;
        Vector3f position;
        Quaternion<float> quat;
        Material material;

    public:
        MeshObject(Mesh mesh, Material material);

    public:
        Matrix4x4f GetObjectMatrix() const;
    };

}