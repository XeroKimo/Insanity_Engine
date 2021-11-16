#pragma once
#include "CommonInclude.h"
#include "Resources.h"
#include "../Factories/ResourceFactory.h"
#include "../Factories/ComponentFactory.h"
#include "Renderer/Renderer.h"




namespace InsanityEngine::DX11
{
    class Device;

    class RenderModule
    {
    private:
        Device& m_device;
        StaticMesh::Renderer m_renderer;
        ComPtr<ID3D11SamplerState> m_defaultSampler;

    public:
        RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device);

    public:
        void Update(float deltaTime);
        void Draw();

    public:
        std::shared_ptr<Resource<Texture>> CreateTexture(const ResourceInitializer<Texture>& initilaizer);
        std::shared_ptr<Resource<Mesh>> CreateMesh(const ResourceInitializer<Mesh>& initilaizer);
        std::shared_ptr<Resource<Shader>> CreateShader(const ResourceInitializer<Shader>& initilaizer);
        std::shared_ptr<Resource<StaticMesh::Material>> CreateMaterial(const ResourceInitializer<StaticMesh::Material>& initializer);
    };

}