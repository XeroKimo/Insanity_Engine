#pragma once
#include "../Application/Window.h"
#include "../DX11/Mesh.h"
#include "../Engine/Camera.h"
#include <array>
#include <unordered_map>

namespace InsanityEngine::Application
{
    class Renderer;



    class CameraObject
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    private:
        ComPtr<ID3D11Buffer> m_cameraConstants;

    public:
        InsanityEngine::Engine::Camera camera;

    public:
        CameraObject(ComPtr<ID3D11Buffer> cameraConstants, InsanityEngine::Engine::Camera&& camera);


    public:

        ID3D11Buffer* GetConstantBuffer() const { return m_cameraConstants.Get(); }
    };



    class MeshObject
    {
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    private:
        ComPtr<ID3D11Buffer> m_objectConstants;

    public:
        DX11::StaticMesh::MeshObject mesh;

    public:
        MeshObject(ComPtr<ID3D11Buffer> constantBuffer, DX11::StaticMesh::MeshObject&& mesh);

    public:
        ID3D11Buffer* GetConstantBuffer() const { return m_objectConstants.Get(); }
    };



    template<class T>
    class Handle;

    template<>
    class Handle<CameraObject>
    {
    private:
        Renderer* m_renderer = nullptr;
        CameraObject* m_object = nullptr;

    public:
        Handle() = default;
        Handle(std::nullptr_t) {}
        Handle(Renderer& renderer, CameraObject& object);
        Handle(const Handle& other) = delete;
        Handle(Handle&& other) noexcept :
            m_renderer(std::move(other.m_renderer)),
            m_object(std::move(other.m_object))
        {
            other.m_renderer = nullptr;
            other.m_object = nullptr;

        };
        ~Handle();

        Handle operator=(const Handle& other) = delete;
        Handle& operator=(Handle&& other) noexcept
        {
            m_renderer = std::move(other.m_renderer);
            m_object = std::move(other.m_object);
            other.m_renderer = nullptr;
            other.m_object = nullptr;
            return *this;
        }

        Handle& operator=(std::nullptr_t)
        {
            Handle copy = std::move(*this);
            return *this;
        }

    };


    template<>
    class Handle<MeshObject>
    {
    private:
        Renderer* m_renderer = nullptr;
        MeshObject* m_object = nullptr;

    public:
        Handle() = default;
        Handle(std::nullptr_t) {}
        Handle(Renderer& renderer, MeshObject& object);
        Handle(const Handle& other) = delete;
        Handle(Handle&& other) noexcept :
            m_renderer(std::move(other.m_renderer)),
            m_object(std::move(other.m_object))
        {
            other.m_renderer = nullptr;
            other.m_object = nullptr;
        };
        ~Handle();

        Handle& operator=(const Handle& other) = delete;
        Handle& operator=(Handle&& other) noexcept
        {
            m_renderer = std::move(other.m_renderer);
            m_object = std::move(other.m_object);
            other.m_renderer = nullptr;
            other.m_object = nullptr;
            return *this;
        }

        Handle& operator=(std::nullptr_t)
        {
            Handle copy = std::move(*this);
            return *this;
        }

    public:
        void SetPosition(Math::Types::Vector3f position)
        {
            m_object->mesh.position = position;
        }

        void SetRotation(Math::Types::Quaternion<float> rotation)
        {
            m_object->mesh.rotation = rotation;
        }

        void Rotate(Math::Types::Quaternion<float> rotation)
        {
            m_object->mesh.rotation *= rotation;
        }

        std::shared_ptr<DX11::StaticMesh::Material> GetMaterial() { return m_object->mesh.GetMaterial(); }
    };


    using CameraHandle = Handle<CameraObject>;
    using MeshHandle = Handle<MeshObject>;


    class Renderer : public BaseRenderer
    {
        static constexpr size_t positionNormalUVLayout = 0;

        template<class T>
        friend class Handle;

    private:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        DX11::Device* m_device = nullptr;
        std::array<ComPtr<ID3D11InputLayout>, 1> m_inputLayouts;

        std::vector<std::unique_ptr<CameraObject>> m_cameras;
        std::vector<std::unique_ptr<MeshObject>> m_meshes;
    public:
        Renderer(DX11::Device& device);

    public:
        CameraHandle CreateCamera(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil = nullptr, ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr);
        MeshHandle CreateMesh(std::shared_ptr<DX11::StaticMesh::Mesh> mesh, std::shared_ptr<DX11::StaticMesh::Material> material);

    protected:
        void Draw(ComPtr<ID3D11RenderTargetView1> backBuffer) override;

    private:
        void Destroy(CameraObject* object);
        void Destroy(MeshObject* object);
    };


    inline Handle<CameraObject>::Handle(Renderer& renderer, CameraObject& object) :
        m_renderer(&renderer),
        m_object(&object)
    {
    }

    inline Handle<MeshObject>::Handle(Renderer& renderer, MeshObject& object) :
        m_renderer(&renderer),
        m_object(&object)
    {
    }

    inline Handle<CameraObject>::~Handle()
    {
        if(m_renderer != nullptr)
            m_renderer->Destroy(m_object);
    }

    inline Handle<MeshObject>::~Handle()
    {
        if(m_renderer != nullptr)
            m_renderer->Destroy(m_object);
    }
}