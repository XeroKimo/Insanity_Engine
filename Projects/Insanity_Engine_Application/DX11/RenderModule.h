#pragma once
#include "CommonInclude.h"
#include "Resources.h"
#include "../Factories/ResourceFactory.h"
#include "../Factories/ComponentFactory.h"
#include "Components/Camera.h"
#include "Renderer/Renderer.h"




namespace InsanityEngine::DX11
{
    class Device;

    class RenderModule
    {
        template<class ObjectT, class Renderer>
        friend struct ManagedHandleDeleter;
    private:
        StaticMesh::Renderer m_renderer;
        ComPtr<ID3D11SamplerState> m_defaultSampler;
        ComPtr<ID3D11DepthStencilState> m_defaultDepthStencilState;
        ComPtr<IDXGISwapChain4> m_swapChain;

        Device& m_device;
        Window& m_window;

    private:
        std::vector<std::unique_ptr<Camera>> m_cameras;

        Camera* m_mainCamera;

    public:
        RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device, Window& window);

    public:
        void Update(float deltaTime);
        void Draw();

    public:
        std::shared_ptr<Resource<Texture>> CreateTexture(const ResourceInitializer<Texture>& initilaizer);
        std::shared_ptr<Resource<Mesh>> CreateMesh(const ResourceInitializer<Mesh>& initilaizer);
        std::shared_ptr<Resource<Shader>> CreateShader(const ResourceInitializer<Shader>& initilaizer);
        std::shared_ptr<Resource<StaticMesh::Material>> CreateMaterial(const ResourceInitializer<StaticMesh::Material>& initializer);

        Component<Camera> CreateCamera(const ComponentInitializer<Camera>& initializer);

    private:
        void Destroy(Camera* camera);
    };

}