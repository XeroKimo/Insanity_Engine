#include "RenderModule.h"
#include "Device.h"
#include "Helpers.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "ShaderConstants.h"

namespace InsanityEngine::DX11
{
    RenderModule::RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device) :
        m_device(device),
        m_renderer(device)
    {
        resourceFactory.AddResourceCreationCallback<Texture>([&](const ResourceInitializer<Texture>& init) { return this->CreateTexture(init); });
        resourceFactory.AddResourceCreationCallback<Mesh>([&](const ResourceInitializer<Mesh>& init) { return this->CreateMesh(init); });
        resourceFactory.AddResourceCreationCallback<Shader>([&](const ResourceInitializer<Shader>& init) { return this->CreateShader(init); });
        resourceFactory.AddResourceCreationCallback<StaticMesh::Material>([&](const ResourceInitializer<StaticMesh::Material>& init) { return this->CreateMaterial(init); });

        componentFactory.RegisterComponentCreationCallback<StaticMesh::MeshObject>([&](const ComponentInitializer<StaticMesh::MeshObject>& init) { return m_renderer.CreateMesh(init.data); });

        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = -FLT_MAX;
        samplerDesc.MaxLOD = FLT_MAX;

        HRESULT hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &m_defaultSampler);
        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create sampler state", hr);
        }
    }
    void RenderModule::Update(float deltaTime)
    {
        m_renderer.Update();
    }
    void RenderModule::Draw()
    {
        m_renderer.Draw();
    }

    std::shared_ptr<Resource<Texture>> RenderModule::CreateTexture(const ResourceInitializer<Texture>& initializer)
    {
        return std::make_shared<Resource<Texture>>(initializer.name, DX11::CreateTexture(m_device.GetDevice(), initializer.textureName, m_defaultSampler));
    }

    std::shared_ptr<Resource<Mesh>> RenderModule::CreateMesh(const ResourceInitializer<Mesh>& initializer)
    {
        using init_type = ResourceInitializer<Mesh>;
        if(std::holds_alternative<init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>>(initializer.data))
        {
            const init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>& mesh = std::get<init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>>(initializer.data);

            ComPtr<ID3D11Buffer> vertexBuffer;
            HRESULT hr = Helpers::CreateVertexBuffer(m_device.GetDevice(), &vertexBuffer, mesh.vertices, Helpers::VertexBufferUsage::Immutable);

            if(FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create Vertex Buffer", hr);
            }

            ComPtr<ID3D11Buffer> indexBuffer;
            hr = Helpers::CreateIndexBuffer(m_device.GetDevice(), &indexBuffer, mesh.indices);

            if(FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create Index Buffer", hr);
            }

            Mesh m(vertexBuffer, indexBuffer, static_cast<UINT>(mesh.vertices.size()), static_cast<UINT>(mesh.indices.size()));
            return std::make_shared<Resource<Mesh>>(initializer.name, m);

        }

        return nullptr;
    }

    std::shared_ptr<Resource<Shader>> RenderModule::CreateShader(const ResourceInitializer<Shader>& initializer)
    {

        return std::make_shared<Resource<Shader>>(initializer.name, DX11::CreateShader(m_device.GetDevice(), initializer.vertexShader, initializer.pixelShader));
    }
    std::shared_ptr<Resource<StaticMesh::Material>> RenderModule::CreateMaterial(const ResourceInitializer<StaticMesh::Material>& initializer)
    {
            ComPtr<ID3D11Buffer> constantBuffer;
            DX11::StaticMesh::Constants::PSMaterial constants{ .color = initializer.color };
            Helpers::CreateConstantBuffer(m_device.GetDevice(), &constantBuffer, true, constants);

            StaticMesh::Material m(constantBuffer, initializer.shader, initializer.albedo, initializer.color);
            return std::make_shared<Resource<StaticMesh::Material>>(initializer.name, m);
    }
}