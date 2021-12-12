#pragma once

#include "CommonInclude.h"
#include "Components/StaticMeshInstance.h"
#include "Components/Camera.h"
#include "InputLayouts.h"
#include <span>

namespace InsanityEngine::DX11
{
    class Device;

    class Renderer;

    class DrawCallbacks
    {
    public:
        virtual void OnBinded(Renderer& renderer) = 0;
        virtual void OnUpdate(Renderer& renderer) = 0;
        virtual void OnDraw(Renderer& renderer) = 0;
        virtual void OnUnbinded(Renderer& renderer) = 0;
    };

    class Renderer
    {
    private:
        Device* m_device = nullptr;
        DrawCallbacks* m_drawCallbacks = nullptr;
        ComPtr<IDXGISwapChain4> m_swapChain;
        ComPtr<ID3D11RenderTargetView1> m_backBuffer;

        ComPtr<ID3D11SamplerState> m_samplerState;
        ComPtr<ID3D11DepthStencilState> m_depthState;

        ComPtr<ID3D11InputLayout> m_staticMeshInputLayout;
        std::vector<std::unique_ptr<Component<StaticMesh::Instance>>> m_meshInstances;

    public:
        Renderer(Device& device, ComPtr<IDXGISwapChain> swapChain);
        ~Renderer();

    public:
        void SetDrawer(DrawCallbacks& drawCallback)
        {
            if(m_drawCallbacks != nullptr)
                m_drawCallbacks->OnUnbinded(*this);

            m_drawCallbacks = &drawCallback;

            m_drawCallbacks->OnBinded(*this);
        }

        void Update();
        void Draw();

    public:
        ID3D11Device5& GetDevice() const;
        ID3D11DeviceContext4& GetDeviceContext() const;
        IDXGISwapChain4& GetSwapChain() const { return *m_swapChain.Get(); }

    public:
        void UpdateCameraData(Component<Camera>& camera);
        void ClearCameraBuffer(Component<Camera>& camera, Math::Types::Vector4f color);
        void RenderMeshes(Component<Camera>& camera);

    public:
        ResourceHandle<Mesh> CreateStaticMesh(std::span<InputLayouts::PositionNormalUV::VertexData> vertices, std::span<UINT> indices);
        ResourceHandle<StaticMesh::Material> CreateMaterial(ResourceHandle<Texture> texture, ResourceHandle<Shader> shader);
        ResourceHandle<Texture> CreateTexture(std::wstring_view fileName);
        ResourceHandle<Shader> CreateShader(std::wstring_view vertexShader, std::wstring_view pixelShader);

        ComponentHandle<StaticMesh::Instance> Create(ResourceHandle<Mesh> mesh, ResourceHandle<StaticMesh::Material> material);
        Component<DX11::Camera> CreateCamera();
        void Destroy(Component<StaticMesh::Instance>* mesh) {}

    private:
        void InitializeBackBuffer();
    };
}