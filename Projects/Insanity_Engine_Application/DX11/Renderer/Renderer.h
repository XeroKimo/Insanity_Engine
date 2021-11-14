#pragma once
#include "../Window.h"
#include "../Resources.h"
#include "Handle.h"
#include "Camera.h"
#include "MeshObject.h"
#include <array>
#include <unordered_map>

namespace InsanityEngine::DX11::StaticMesh
{
    class Renderer;

    //using CameraHandle = Handle<CameraObject>;
    using StaticMeshHandle = StaticMesh::MeshHandle;


    class Renderer
    {
        static constexpr size_t positionNormalUVLayout = 0;

        template<class ObjectT, class Renderer>
        friend struct ManagedHandleDeleter;

    private:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        DX11::Device* m_device = nullptr;
        std::array<ComPtr<ID3D11InputLayout>, 1> m_inputLayouts;
        std::vector<std::unique_ptr<StaticMesh::MeshObject>> m_meshes;
    public:
        Renderer(DX11::Device& device);

    public:
        Component<MeshObject> CreateMesh(DX11::StaticMesh::MeshObjectData data);

    public:
        void Update();
        void Draw();

    private:
        void Destroy(StaticMesh::MeshObject* object);
    };
}