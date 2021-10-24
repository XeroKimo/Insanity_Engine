#include "Mesh.h"
#include "Extensions/MatrixExtension.h"
#include "Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "Helpers.h"
#include "ShaderConstants.h"
#include <assert.h>


namespace InsanityEngine::DX11::StaticMesh
{
    using namespace Math::Types;

    Mesh::Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount) :
        m_vertexBuffer(std::move(vertexBuffer)),
        m_vertexCount(std::move(vertexCount)),
        m_indexBuffer(std::move(indexBuffer)),
        m_indexCount(std::move(indexCount))
    {
        assert(m_vertexBuffer != nullptr);
        assert(m_indexBuffer != nullptr);
    }


    Texture::Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState) :
        m_shaderResourceView(std::move(shaderResourceView)),
        m_samplerState(std::move(samplerState))
    {
        assert(m_shaderResourceView != nullptr);
        assert(m_samplerState != nullptr);
    }

    void Texture::SetSamplerState(ComPtr<ID3D11SamplerState> samplerState)
    {
        assert(samplerState != nullptr);

        m_samplerState = std::move(samplerState);
    }

    Shader::Shader(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader) :
        m_vertexShader(std::move(vertexShader)),
        m_pixelShader(std::move(pixelShader))
    {
        assert(m_vertexShader != nullptr);
        assert(m_pixelShader != nullptr);
    }

    std::shared_ptr<Shader> Material::defaultShader = nullptr;
    std::shared_ptr<Texture> Material::defaultAlbedo = nullptr;

    Material::Material(std::shared_ptr<Shader> shader, std::shared_ptr<Texture> albedo, ComPtr<ID3D11Buffer> materialConstantBuffer, Vector4f color) :
        m_shader((shader == nullptr) ? defaultShader : std::move(shader)),
        m_albedo((albedo == nullptr) ? defaultAlbedo : std::move(albedo)),
        m_materialConstantBuffer(std::move(materialConstantBuffer)),
        color(color)
    {
        assert(m_shader != nullptr);
        assert(m_materialConstantBuffer != nullptr);
        assert(m_albedo != nullptr);
    }

    void Material::SetShader(std::shared_ptr<Shader> shader)
    {
        m_shader = std::move(shader);

        if(m_shader == nullptr)
            m_shader = defaultShader;

        assert(m_shader != nullptr);
    }

    void Material::SetAlbedo(std::shared_ptr<Texture> albedo)
    {
        m_albedo = std::move(albedo);

        if(m_albedo == nullptr)
            m_albedo = defaultAlbedo;

        assert(m_albedo != nullptr);

    }

    std::shared_ptr<Mesh> MeshObject::defaultMesh = nullptr;
    std::shared_ptr<Material> MeshObject::defaultMaterial = nullptr;

    MeshObject::MeshObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
        m_mesh((mesh == nullptr) ? defaultMesh : std::move(mesh)),
        m_material((material == nullptr) ? defaultMaterial : std::move(material))
    {
        assert(m_mesh != nullptr && m_material != nullptr);
    }

    void MeshObject::SetMesh(std::shared_ptr<Mesh> mesh)
    {
        assert(mesh != nullptr);
        m_mesh = std::move(mesh);
    }

    void MeshObject::SetMaterial(std::shared_ptr<Material> material)
    {
        assert(material != nullptr);
        m_material = std::move(material);
    }

    Matrix4x4f MeshObject::GetObjectMatrix() const
    {
        return Math::Matrix::PositionMatrix(position) * rotation.ToRotationMatrix();
    }

    std::shared_ptr<Shader> CreateShader(ID3D11Device* device, std::wstring_view vertexShader, std::wstring_view pixelShader)
    {
        ComPtr<ID3DBlob> data;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3DCompileFromFile(
            vertexShader.data(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "vs_5_0",
            0,
            0,
            &data,
            &error);


        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to compile vertex shader: " + std::string(errorMsg), hr);
        }


        ComPtr<ID3D11VertexShader> vs;
        hr = device->CreateVertexShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &vs);

        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to create vertex shader: " + std::string(errorMsg), hr);
        }

        hr = D3DCompileFromFile(
            pixelShader.data(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ps_5_0",
            0,
            0,
            &data,
            &error);

        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to compile pixel shader: " + std::string(errorMsg), hr);
        }


        ComPtr<ID3D11PixelShader> ps;
        hr = device->CreatePixelShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &ps);
        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to create pixel shader:" + std::string(errorMsg), hr);
        }

        return std::make_shared<Shader>(vs, ps);
    }

    std::shared_ptr<Mesh> CreateMesh(ID3D11Device* device, std::span<VertexData> vertices, std::span<UINT> indices)
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        HRESULT hr = Helpers::CreateVertexBuffer(device, &vertexBuffer, vertices, Helpers::VertexBufferUsage::Immutable);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create Vertex Buffer", hr);
        }

        ComPtr<ID3D11Buffer> indexBuffer;
        hr = Helpers::CreateIndexBuffer(device, &indexBuffer, indices);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create Index Buffer", hr);
        }

        return std::make_shared<Mesh>(vertexBuffer, static_cast<UINT>(vertices.size()), indexBuffer, static_cast<UINT>(indices.size()));
    }

    std::shared_ptr<Texture> CreateTexture(ID3D11Device* device, std::wstring_view texture, ComPtr<ID3D11SamplerState> sampler)
    {
        ComPtr<ID3D11ShaderResourceView> resource;
        HRESULT hr = Helpers::CreateTextureFromFile(device, &resource, texture, DirectX::WIC_FLAGS_NONE);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create Index Buffer", hr);
        }

        return std::make_shared<Texture>(resource, sampler);
    }

    std::shared_ptr<Material> CreateMaterial(ID3D11Device* device, std::shared_ptr<Shader> shader, std::shared_ptr<Texture> texture, Math::Types::Vector4f color)
    {
        ComPtr<ID3D11Buffer> constantBuffer;
        StaticMesh::Constants::PSMaterial constants{ .color = color };
        Helpers::CreateConstantBuffer(device, &constantBuffer, true, constants);
        return std::make_shared<Material>(shader, texture, constantBuffer, color);
    }

}