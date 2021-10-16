#pragma once
#include "Mesh.h"

namespace InsanityEngine::DX11
{
    class Device;
}

namespace InsanityEngine::DX11::Renderers
{
    namespace Registers
    {
        namespace VS
        {
            namespace Shared
            {
                inline constexpr UINT appConstants = 0;
                inline constexpr UINT cameraConstants = 1;
            }

            namespace StaticMesh
            {
                inline constexpr UINT objectConstants = 2;
            }
        }

        namespace PS
        {
            namespace Shared
            {
                inline constexpr UINT appConstants = 0;

            }

            namespace StaticMesh
            {
                inline constexpr UINT albedoTexture = 0;
                inline constexpr UINT albedoSampler = 0;

                inline constexpr UINT materialConstants = 2;
            }
        }
    };

    namespace Shared
    {
        struct ApplicationConstants
        {

        };

        struct CameraConstants
        {
            Math::Types::Matrix4x4f viewProjMatrix;
        };
    }

    namespace StaticMesh
    {
        using namespace DX11::StaticMesh;

        struct VSObjectConstants
        {
            Math::Types::Matrix4x4f worldMatrix;
        };

        struct PSObjectConstants
        {
            Math::Types::Vector4f color;
        };

        class RenderObject
        {
            ComPtr<ID3D11Buffer> m_objectConstantBuffer;

        public:
            MeshObject object;

        public:
            RenderObject(ComPtr<ID3D11Buffer> constantBuffer, MeshObject object) :
                m_objectConstantBuffer(constantBuffer),
                object(object)
            {

            }

        public:
            ID3D11Buffer* GetConstantBuffer() const { return m_objectConstantBuffer.Get(); }
        };

        class Renderer
        {
            Device* m_device;
            std::vector<std::unique_ptr<RenderObject>> m_renderObjects;

        public:
            Renderer(Device& device);

            RenderObject* CreateObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
            void DestroyObject(RenderObject*& object);

        public:
            const std::vector<std::unique_ptr<RenderObject>>& GetRenderObjects() const { return m_renderObjects; }
            const Device& GetDevice() const { return *m_device; }

        private:
            ComPtr<ID3D11Buffer> CreateMeshConstantBuffer();
        };
    };
}