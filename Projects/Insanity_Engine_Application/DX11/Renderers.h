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
            namespace StaticMesh
            {
                inline constexpr UINT appConstants = 0;
                inline constexpr UINT cameraConstants = 1;
                inline constexpr UINT objectConstants = 2;
            }
        }

        namespace PS
        {
            namespace StaticMesh
            {
                inline constexpr UINT albedoTexture = 0;
                inline constexpr UINT albedoSampler = 0;

                inline constexpr UINT appConstants = 0;
                inline constexpr UINT materialConstants = 2;
            }
        }
    };
    namespace StaticMesh
    {
        using namespace DX11::StaticMesh;

        struct ApplicationConstants
        {

        };

        struct CameraConstants
        {
            Math::Types::Matrix4x4f viewProjMatrix;
        };

        struct VSObjectConstants
        {
            Math::Types::Matrix4x4f worldMatrix;
        };

        struct PSMaterialConstants
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



        class Renderer;



        struct RenderObjectDeleter
        {
            Renderer* renderer = nullptr;

            RenderObjectDeleter() = default;
            RenderObjectDeleter(Renderer* renderer);

            void operator()(RenderObject* renderObject);
        };

        using MeshHandle = std::unique_ptr<RenderObject, RenderObjectDeleter>;

        class Renderer
        {
            friend void RenderObjectDeleter::operator()(RenderObject*);

            Device* m_device;
            std::vector<std::unique_ptr<RenderObject>> m_renderObjects;

        public:
            Renderer(Device& device);

            MeshHandle CreateObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
            void DestroyObject(RenderObject*& object);

        public:
            const std::vector<std::unique_ptr<RenderObject>>& GetRenderObjects() const { return m_renderObjects; }
            const Device& GetDevice() const { return *m_device; }

        private:
            ComPtr<ID3D11Buffer> CreateMeshConstantBuffer();
        };
    };
}