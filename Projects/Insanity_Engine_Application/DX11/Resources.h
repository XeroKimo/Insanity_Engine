#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"
#include "InputLayouts.h"
#include "../Factories/ResourceFactory.h"
#include "Resources/Texture.h"
#include "Resources/Shader.h"
#include <array>
#include <span>
#include <variant>

namespace InsanityEngine::DX11
{
    class Device;
}
//
namespace InsanityEngine::DX11
{
//    class Mesh
//    {
//    private:
//        ComPtr<ID3D11Buffer> m_vertexBuffer;
//        ComPtr<ID3D11Buffer> m_indexBuffer;
//
//        UINT m_vertexCount = 0;
//        UINT m_indexCount = 0;
//
//    public:
//        Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount);
//        Mesh(const Mesh& other) = delete;
//        Mesh(Mesh&& other) noexcept = default;
//        ~Mesh() = default;
//
//    public:
//        Mesh& operator=(const Mesh& other) = delete;
//        Mesh& operator=(Mesh&& other) noexcept = default;
//
//    public:
//        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
//        ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }
//
//        UINT GetVertexCount() const { return m_vertexCount; }
//        UINT GetIndexCount() const { return m_indexCount; }
//    };
//
//
//
//    class Texture
//    {
//        ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
//        ComPtr<ID3D11SamplerState> m_samplerState;
//
//    public:
//        Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState);
//        Texture(const Texture& other) = delete;
//        Texture(Texture&& other) noexcept = default;
//        ~Texture() = default;
//
//    public:
//        Texture& operator=(const Texture& other) = delete;
//        Texture& operator=(Texture&& other) noexcept = default;
//
//    public:
//        void SetSamplerState(ComPtr<ID3D11SamplerState> samplerState);
//
//        ID3D11ShaderResourceView* GetView() const { return m_shaderResourceView.Get(); }
//        ID3D11SamplerState* GetSamplerState() const { return m_samplerState.Get(); }
//    };
//
//
//
//    class Shader
//    {
//    private:
//        ComPtr<ID3D11VertexShader> m_vertexShader;
//        ComPtr<ID3D11PixelShader> m_pixelShader;
//
//    public:
//        Shader(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader);
//        Shader(const Shader& other) = delete;
//        Shader(Shader&& other) noexcept = default;
//        ~Shader() = default;
//
//    public:
//        Shader& operator=(const Shader& other) = delete;
//        Shader& operator=(Shader&& other) noexcept = default;
//
//    public:
//        ID3D11VertexShader* GetVertexShader() const { return m_vertexShader.Get(); }
//        ID3D11PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }
//    };
//
//    namespace StaticMesh
//    {
//
//        class Material
//        {
//        private:
//            ComPtr<ID3D11Buffer> m_constantBuffer;
//            std::shared_ptr<Resource<Shader>> m_shader;
//            std::shared_ptr<Resource<Texture>> m_albedo;
//            Math::Types::Vector4f m_color;
//
//        public:
//            Material(ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Resource<Shader>> shader, std::shared_ptr<Resource<Texture>> albedo, Math::Types::Vector4f color = Math::Types::Vector4f(Math::Types::Scalar(1.f)));
//            Material(const Material& other) = delete;
//            Material(Material&& other) noexcept = default;
//            ~Material() = default;
//
//        public:
//            Material& operator=(const Material& other) = delete;
//            Material& operator=(Material&& other) noexcept = default;
//
//        public:
//            void SetShader(std::shared_ptr<Resource<Shader>> shader);
//            void SetAlbedo(std::shared_ptr<Resource<Texture>> texture);
//            void SetColor(Math::Types::Vector4f color);
//
//        public:
//            std::shared_ptr<Resource<Shader>> GetShader() const { return m_shader; }
//            std::shared_ptr<Resource<Texture>> GetAlbedo() const { return m_albedo; }
//            Math::Types::Vector4f GetColor() const { return m_color; }
//
//            ID3D11Buffer* GetConstantBuffer() const { return m_constantBuffer.Get(); }
//        public:
//            friend bool operator==(const Material& lh, const Material& rh) = default;
//            friend bool operator!=(const Material& lh, const Material& rh) = default;
//
//        };
//
//        extern StaticMesh::Material CreateMaterial(ID3D11Device* device, std::shared_ptr<Resource<Shader>> shader, std::shared_ptr<Resource<Texture>> texture, Math::Types::Vector4f color = Math::Types::Vector4f(Math::Types::Scalar(1.f)));
//    }
//
    extern Shader CreateShader(ID3D11Device* device, std::wstring_view vertexShader, std::wstring_view pixelShader);
    extern Texture CreateTexture(ID3D11Device* device, std::wstring_view texture, ComPtr<ID3D11SamplerState> sampler);

}
//
//
//namespace InsanityEngine::DX11::StaticMesh
//{
//    using VertexData = DX11::InputLayouts::PositionNormalUV::VertexData;
//
//    extern Resources::Mesh CreateMesh(ID3D11Device* device, std::span<VertexData> vertices, std::span<UINT> indices);
//}
//
//template<>
//struct InsanityEngine::ResourceInitializer<InsanityEngine::DX11::Resources::Texture> : InsanityEngine::ResourceInitializer<UnknownResource>
//{
//    std::wstring_view textureName;
//    DX11::ComPtr<ID3D11SamplerState> sampler;
//};
//
//template<>
//struct InsanityEngine::ResourceInitializer<InsanityEngine::DX11::Resources::Mesh> : InsanityEngine::ResourceInitializer<UnknownResource>
//{
//    struct StaticMeshRaw
//    {
//        std::span<DX11::StaticMesh::VertexData> vertices;
//        std::span<UINT> indices;
//    };
//    
//    std::variant<StaticMeshRaw> data;
//
//};
//
//template<>
//struct InsanityEngine::ResourceInitializer<InsanityEngine::DX11::Resources::Shader> : InsanityEngine::ResourceInitializer<UnknownResource>
//{
//    std::wstring_view vertexShader;
//    std::wstring_view pixelShader;
//};
