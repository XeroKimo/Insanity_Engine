#include "Application.h"
#include "../DX11/RenderModule.h"
#include "../DX11/Renderer.h"
#include "../DX11/Components/StaticMeshInstance.h"

namespace InsanityEngine::Application
{
    class ApplicationDrawer : public DX11::DrawCallbacks
    {
        Component<DX11::Camera> m_camera;
    public:
        void OnBinded(DX11::Renderer& renderer) override
        {
            m_camera = renderer.CreateCamera();
            m_camera.data.position.z() = -5;
        }
        void OnUpdate(DX11::Renderer& renderer)
        {
            renderer.UpdateCameraData(m_camera);
        }

        void OnDraw(DX11::Renderer& renderer) override
        {
            renderer.ClearCameraBuffer(m_camera, { 0, 0.3f, 0.7f, 1 });
            renderer.RenderMeshes(m_camera);
        }
        void OnUnbinded(DX11::Renderer& renderer) override
        {

        }
    };


    using namespace Math::Types;

    int Run()
    {
        DX11::RenderModule renderModule;
        DX11::Window<DX11::Renderer> window = renderModule.WindowCreate<DX11::Renderer>("Test window", { 1280, 720 });

        auto vertices = std::to_array(
            {
                DX11::InputLayouts::PositionNormalUV::VertexData{ Vector3f(-0.5f, -0.5f, 0), Vector3f(), Vector2f(0, 0) },
                DX11::InputLayouts::PositionNormalUV::VertexData{ Vector3f(0, 0.5f, 0), Vector3f(), Vector2f(0.5f, 1) },
                DX11::InputLayouts::PositionNormalUV::VertexData{ Vector3f(0.5f, -0.5f, 0), Vector3f(), Vector2f(1, 0) }
            });

        auto indices = std::to_array<UINT>(
            {
                0, 1, 2
            });

        ApplicationDrawer drawer;
        window.GetRenderer().SetDrawer(drawer);

        ResourceHandle<DX11::Mesh> mesh = window.GetRenderer().CreateStaticMesh(vertices, indices);
        ResourceHandle<DX11::Shader> shader = window.GetRenderer().CreateShader(L"Resources/Shaders/VertexShader.hlsl", L"Resources/Shaders/PixelShader.hlsl");
        ResourceHandle<DX11::Texture> texture = window.GetRenderer().CreateTexture(L"Resources/Korone_NotLikeThis.png");
        ResourceHandle<DX11::StaticMesh::Material> material = window.GetRenderer().CreateMaterial(texture, shader);

        ComponentHandle<DX11::StaticMesh::Instance> instance = window.GetRenderer().Create(mesh, material);



        SDL_Event event;
        while(true)
        {
            if(SDL_PollEvent(&event))
            {
                if(event.type == SDL_EventType::SDL_QUIT)
                    break;
            }
            else
            {

                window.GetRenderer().Update();
                window.GetRenderer().Draw();
            }
        }

        return 0;
    }
}
