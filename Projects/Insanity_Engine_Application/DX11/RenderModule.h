#pragma once
#include "CommonInclude.h"
#include "Resources.h"
#include "../ResourceFactory.h"
#include "../ComponentFactory.h"
#include "Renderer/Renderer.h"




namespace InsanityEngine::DX11
{
    class Device;

    class RenderModule
    {
    private:
        Device& m_device;
        StaticMesh::Renderer m_renderer;


    public:
        RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device);

    public:
        void Update(float deltaTime);
        void Draw();

    public:
        std::shared_ptr<Resource<Resources::Texture>> CreateTexture(const ResourceInitializer<Resources::Texture>& initilaizer);
        std::shared_ptr<Resource<Resources::Mesh>> CreateMesh(const ResourceInitializer<Resources::Mesh>& initilaizer);
        std::shared_ptr<Resource<Resources::Shader>> CreateShader(const ResourceInitializer<Resources::Shader>& initilaizer);

    };

}