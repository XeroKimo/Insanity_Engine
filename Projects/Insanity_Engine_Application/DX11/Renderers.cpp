#include "Renderers.h"
#include "Helpers.h"
#include "Device.h"

namespace InsanityEngine::DX11::Renderers
{
    namespace StaticMesh
    {
        Renderer::Renderer(Device& device) :
            m_device(&device)
        {
        }

        MeshHandle Renderer::CreateObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
        {
            m_renderObjects.push_back(std::make_unique<RenderObject>(CreateMeshConstantBuffer(), MeshObject(mesh, material)));
            return { m_renderObjects.back().get(), RenderObjectDeleter(this) };
        }

        void Renderer::DestroyObject(RenderObject*& object)
        {
            auto it = std::remove_if(m_renderObjects.begin(), m_renderObjects.end(), [&](const std::unique_ptr<RenderObject>& ptr) { return ptr.get() == object; });
            m_renderObjects.erase(it, m_renderObjects.end());
            object = nullptr;
        }


        ComPtr<ID3D11Buffer> Renderer::CreateMeshConstantBuffer()
        {
            ComPtr<ID3D11Buffer> buffer;
            Helpers::CreateConstantBuffer<VSObjectConstants>(m_device->GetDevice(), &buffer, true);
            return buffer;
        }


        RenderObjectDeleter::RenderObjectDeleter(Renderer* renderer) :
            renderer(renderer)
        {
        }

        void RenderObjectDeleter::operator()(RenderObject* renderObject)
        {
            renderer->DestroyObject(renderObject);
        }
    }
}