#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"
#include "InputLayouts.h"
#include <array>
#include <span>

namespace InsanityEngine::DX11
{
    class Device;
}

namespace InsanityEngine::DX11::StaticMesh
{
    using VertexData = DX11::InputLayouts::PositionNormalUV::VertexData;

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
        friend bool operator!=(const Mesh& lh, const Mesh& rh) = default;

    public:
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }

        UINT GetVertexCount() const { return m_vertexCount; }
        UINT GetIndexCount() const { return m_indexCount; }
    };



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



    class Shader
    {
    private:
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;

    public:
        Shader(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader);

    public:
        ID3D11VertexShader* GetVertexShader() const { return m_vertexShader.Get(); }
        ID3D11PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }
    };



    class Material
    {
    public:
        static std::shared_ptr<Texture> defaultAlbedo;
        static std::shared_ptr<Shader> defaultShader;

    private:
        ComPtr<ID3D11Buffer> m_constantBuffer;
        std::shared_ptr<Shader> m_shader;
        std::shared_ptr<Texture> m_albedo;
        Math::Types::Vector4f m_color;

    public:
        Material(ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Shader> shader, std::shared_ptr<Texture> albedo, Math::Types::Vector4f color = Math::Types::Vector4f(Math::Types::Scalar(1.f)));

    public:
        void SetShader(std::shared_ptr<Shader> shader);
        void SetAlbedo(std::shared_ptr<Texture> texture);
        void SetColor(Math::Types::Vector4f color);

    public:
        std::shared_ptr<Shader> GetShader() const { return m_shader; }
        std::shared_ptr<Texture> GetAlbedo() const { return m_albedo; }
        Math::Types::Vector4f GetColor() const { return m_color; }

        ID3D11Buffer* GetConstantBuffer() const { return m_constantBuffer.Get(); }
    public:
        friend bool operator==(const Material& lh, const Material& rh) = default;
        friend bool operator!=(const Material& lh, const Material& rh) = default;

    };



    class MeshObject
    {
    public:
        static std::shared_ptr<Mesh> defaultMesh;
        static std::shared_ptr<Material> defaultMaterial;

    private:
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Material> m_material;

    public:
        Math::Types::Vector3f position;
        Math::Types::Vector3f scale{ Math::Types::Scalar(1.f) };
        Math::Types::Quaternion<float> rotation;

    public:
        MeshObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

    public:
        void SetMesh(std::shared_ptr<Mesh> mesh);
        void SetMaterial(std::shared_ptr<Material> material);

    public:
        std::shared_ptr<Mesh> GetMesh() const { return m_mesh; }
        std::shared_ptr<Material> GetMaterial() const { return m_material; }

    public:
        Math::Types::Matrix4x4f GetObjectMatrix() const;
    };


    extern std::shared_ptr<Shader> CreateShader(ID3D11Device* device, std::wstring_view vertexShader, std::wstring_view pixelShader);
    extern std::shared_ptr<Mesh> CreateMesh(ID3D11Device* device, std::span<VertexData> vertices, std::span<UINT> indices);
    extern std::shared_ptr<Texture> CreateTexture(ID3D11Device* device, std::wstring_view texture, ComPtr<ID3D11SamplerState> sampler);
    extern std::shared_ptr<Material> CreateMaterial(ID3D11Device* device, std::shared_ptr<Shader> shader, std::shared_ptr<Texture> texture, Math::Types::Vector4f color = Math::Types::Vector4f(Math::Types::Scalar(1.f)));

}