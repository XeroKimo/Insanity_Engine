#pragma once
#include "CommonInclude.h"
#include "Resources.h"
#include "../Resource.h"



namespace InsanityEngine::DX11
{
    class Device;

    class RenderModule
    {
    private:
        Device& m_device;

    public:
        RenderModule(ResourceFactory& factory, Device& device);

    public:
        void Update(float deltaTime);
        void Draw();

    public:
        std::shared_ptr<Resource<Resources::Texture>> CreateTexture(const ResourceInitializer<Resources::Texture>& initilaizer);
        std::shared_ptr<Resource<Resources::Mesh>> CreateMesh(const ResourceInitializer<Resources::Mesh>& initilaizer);
        std::shared_ptr<Resource<Resources::Shader>> CreateShader(const ResourceInitializer<Resources::Shader>& initilaizer);

    };

}