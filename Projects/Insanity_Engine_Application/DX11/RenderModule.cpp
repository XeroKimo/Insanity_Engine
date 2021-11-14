#include "RenderModule.h"
#include "Device.h"

namespace InsanityEngine::DX11
{
    RenderModule::RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device) :
        m_device(device),
        m_renderer(device)
    {
        resourceFactory.AddResourceCreationCallback<Resources::Texture>([&](const ResourceInitializer<Resources::Texture>& init) { return this->CreateTexture(init); });
        resourceFactory.AddResourceCreationCallback<Resources::Mesh>([&](const ResourceInitializer<Resources::Mesh>& init) { return this->CreateMesh(init); });
        resourceFactory.AddResourceCreationCallback<Resources::Shader>([&](const ResourceInitializer<Resources::Shader>& init) { return this->CreateShader(init); });

        componentFactory.RegisterComponentCreationCallback<StaticMesh::MeshObject>([&](const ComponentInitializer<StaticMesh::MeshObject>& init) { return m_renderer.CreateMesh(init.data); });
    }
    void RenderModule::Update(float deltaTime)
    {
        m_renderer.Update();
    }
    void RenderModule::Draw()
    {
        m_renderer.Draw();
    }

    std::shared_ptr<Resource<Resources::Texture>> RenderModule::CreateTexture(const ResourceInitializer<Resources::Texture>& initializer)
    {
        return std::make_shared<Resource<Resources::Texture>>(initializer.name, Resources::CreateTexture(m_device.GetDevice(), initializer.textureName, initializer.sampler));
    }

    std::shared_ptr<Resource<Resources::Mesh>> RenderModule::CreateMesh(const ResourceInitializer<Resources::Mesh>& initializer)
    {
        using init_type = ResourceInitializer<Resources::Mesh>;
        if(std::holds_alternative<init_type::StaticMeshRaw>(initializer.data))
        {
            const init_type::StaticMeshRaw& mesh = std::get<init_type::StaticMeshRaw>(initializer.data);
            return std::make_shared<Resource<Resources::Mesh>>(initializer.name, DX11::StaticMesh::CreateMesh(m_device.GetDevice(), mesh.vertices, mesh.indices));

        }

        return nullptr;
    }

    std::shared_ptr<Resource<Resources::Shader>> RenderModule::CreateShader(const ResourceInitializer<Resources::Shader>& initializer)
    {
        return std::make_shared<Resource<Resources::Shader>>(initializer.name, Resources::CreateShader(m_device.GetDevice(), initializer.vertexShader, initializer.pixelShader));
    }
}