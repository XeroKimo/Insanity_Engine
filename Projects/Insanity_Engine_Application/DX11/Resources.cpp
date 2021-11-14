#include "Resources.h"
#include "Extensions/MatrixExtension.h"
#include "Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "Helpers.h"
#include "ShaderConstants.h"
#include <assert.h>


namespace InsanityEngine::DX11::Resources
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



    Shader CreateShader(ID3D11Device* device, std::wstring_view vertexShader, std::wstring_view pixelShader)
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

        return Shader(vs, ps);
    }


    Texture CreateTexture(ID3D11Device* device, std::wstring_view texture, ComPtr<ID3D11SamplerState> sampler)
    {
        ComPtr<ID3D11ShaderResourceView> resource;
        HRESULT hr = Helpers::CreateTextureFromFile(device, &resource, texture, DirectX::WIC_FLAGS_NONE);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create Index Buffer", hr);
        }

        return Texture(resource, sampler);
    }

    namespace StaticMesh
    {
        Material::Material(ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Resource<Shader>> shader, std::shared_ptr<Resource<Texture>> albedo, Vector4f color) :
            m_constantBuffer(std::move(constantBuffer)),
            m_shader(std::move(shader)),
            m_albedo(std::move(albedo)),
            m_color(color)
        {
            assert(m_constantBuffer != nullptr);
            assert(m_shader != nullptr);
            assert(m_albedo != nullptr);
        }

        void Material::SetShader(std::shared_ptr<Resource<Shader>> shader)
        {
            m_shader = std::move(shader);
            assert(shader != nullptr);
        }

        void Material::SetAlbedo(std::shared_ptr<Resource<Texture>> albedo)
        {
            m_albedo = std::move(albedo);
            assert(m_albedo != nullptr);
        }

        void Material::SetColor(Math::Types::Vector4f color)
        {
            m_color = color;
        }


        Material CreateMaterial(ID3D11Device* device, std::shared_ptr<Resource<Shader>> shader, std::shared_ptr<Resource<Texture>> texture, Math::Types::Vector4f color)
        {
            ComPtr<ID3D11Buffer> constantBuffer;
            DX11::StaticMesh::Constants::PSMaterial constants{ .color = color };
            Helpers::CreateConstantBuffer(device, &constantBuffer, true, constants);

            return Material(constantBuffer, shader, texture, color);
        }

    }
}

namespace InsanityEngine::DX11::StaticMesh
{
    using namespace Math::Types;

    Resources::Mesh CreateMesh(ID3D11Device* device, std::span<VertexData> vertices, std::span<UINT> indices)
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

        return Resources::Mesh(vertexBuffer, static_cast<UINT>(vertices.size()), indexBuffer, static_cast<UINT>(indices.size()));
    }
}
//
//InsanityEngine::ResourceInitializer<InsanityEngine::DX11::Resources::Mesh>::ResourceInitializer(std::string_view name, StaticMeshRaw mesh) :
//    InsanityEngine::ResourceInitializer<InsanityEngine::UnknownResource>(name),
//    data(mesh)
//{
//}