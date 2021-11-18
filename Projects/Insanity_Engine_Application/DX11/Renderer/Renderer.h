#pragma once
#include "../Window.h"
#include "../Resources.h"
#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "../Internal/Handle.h"
#include "../Components/Camera.h"
#include "../Components/StaticMeshInstance.h"
#include <array>
#include <unordered_map>

namespace InsanityEngine::DX11::StaticMesh
{
    class Renderer;

    //using CameraHandle = Handle<CameraObject>;


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
        std::vector<std::unique_ptr<StaticMesh::Instance>> m_meshes;
    public:
        Renderer(DX11::Device& device);

    public:
        Component<Instance> CreateMesh(ResourceHandle<Mesh> mesh, ResourceHandle<Material> material);

    public:
        void Update();
        void Draw();

    private:
        void Destroy(StaticMesh::Instance* object);
    };
}