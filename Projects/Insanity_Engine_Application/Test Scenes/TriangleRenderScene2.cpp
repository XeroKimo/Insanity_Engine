#include "TriangleRenderScene2.h"
#include "../DX11/Device.h"
#include "../DX11/Window.h"
#include "../DX11/Renderer/Renderer.h"
#include "../Factories/ResourceFactory.h"
#include "../Factories/ComponentFactory.h"

#include "Debug Classes/Exceptions/HRESULTException.h"

#include <optional>

using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Debug::Exceptions;
using namespace InsanityEngine::Math::Types;

static Component<DX11::StaticMesh::MeshObject> mesh;
static Component<DX11::StaticMesh::MeshObject> mesh2;
static Component<DX11::StaticMesh::MeshObject> mesh3;
static Component<DX11::StaticMesh::MeshObject> mesh4;
static Component<DX11::StaticMesh::MeshObject> mesh5;
//static DX11::StaticMesh::CameraHandle camera;

static ResourceHandle<Mesh> meshRes;
static ResourceHandle<Texture> tex;
static ResourceHandle<Texture> tex2;
static ResourceHandle<Shader> shader;

static ResourceHandle<StaticMesh::Material> mat;
static ResourceHandle<StaticMesh::Material> mat2;
static ResourceHandle<StaticMesh::Material> mat3;
static ResourceHandle<StaticMesh::Material> mat4;
static ResourceHandle<StaticMesh::Material> mat5;

static bool aPressed = false;
static bool wPressed = false;
static bool sPressed = false;
static bool dPressed = false;

static bool upPressed = false;
static bool downPressed = false;
static bool leftPressed = false;
static bool rightPressed = false;

static bool ctrlPressed = false;

void TriangleRenderSetup2(InsanityEngine::DX11::Device& device, InsanityEngine::DX11::Window& window, ResourceFactory& factory, ComponentFactory& componentFactory)
{

    //D3D11_SAMPLER_DESC samplerDesc;
    //samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.BorderColor[0] = 1.0f;
    //samplerDesc.BorderColor[1] = 1.0f;
    //samplerDesc.BorderColor[2] = 1.0f;
    //samplerDesc.BorderColor[3] = 1.0f;
    //samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    //samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    //samplerDesc.MaxAnisotropy = 1;
    //samplerDesc.MipLODBias = 0;
    //samplerDesc.MinLOD = -FLT_MAX;
    //samplerDesc.MaxLOD = FLT_MAX;

    //ComPtr<ID3D11SamplerState> samplerState;
    //HRESULT hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &samplerState);
    //if(FAILED(hr))
    //{
    //    throw HRESULTException("Failed to create sampler state", hr);
    //}

    ResourceInitializer<Shader> shaderData{ "wtf", L"Resources/Shaders/VertexShader.hlsl",  L"Resources/Shaders/PixelShader.hlsl" };
    shader = factory.CreateResource<Shader>(shaderData);


    tex = factory.CreateResource<Texture>({ "Resources/Korone_NotLikeThis.png", L"Resources/Korone_NotLikeThis.png"});
    //tex = std::shared_ptr<Resources::Texture>(test, &test->Get()); //std::make_shared<Resources::Texture>(Resources::CreateTexture(device.GetDevice(), L"Resources/Korone_NotLikeThis.png", samplerState));
    tex2 = factory.CreateResource<Texture>({ "Resources/Dank.png", L"Resources/Dank.png" });

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

    meshRes = factory.CreateResource<Mesh>({ "Mesh", ResourceInitializer<Mesh>::RawInit<InputLayouts::PositionNormalUV::VertexData>{vertices, indices} });

    mat =  factory.CreateResource<StaticMesh::Material>({"Test",  shader, tex });
    mat2 = factory.CreateResource<StaticMesh::Material>({"Test",  shader, tex2, { 1, 0 ,0, 1 } });
    mat3 = factory.CreateResource<StaticMesh::Material>({"Test",  shader, tex2, { 0, 1 ,0, 1 } });
    mat4 = factory.CreateResource<StaticMesh::Material>({"Test",  shader, tex2, { 0, 0 ,1, 1 } });
    mat5 = factory.CreateResource<StaticMesh::Material>({"Test",  shader, tex2, { 1, 1 ,0, 1 } });


    mesh = componentFactory.CreateComponent<DX11::StaticMesh::MeshObject>( { {}, meshRes,  mat });
    mesh2 = componentFactory.CreateComponent<DX11::StaticMesh::MeshObject>({ {}, meshRes,  mat2 });
    mesh3 = componentFactory.CreateComponent<DX11::StaticMesh::MeshObject>({ {}, meshRes,  mat3 });
    mesh4 = componentFactory.CreateComponent<DX11::StaticMesh::MeshObject>({ {}, meshRes,  mat4 });
    mesh5 = componentFactory.CreateComponent<DX11::StaticMesh::MeshObject>({ {}, meshRes,  mat5 });

    mesh.SetPosition({ 0, 0, 2 });
    mesh2.SetPosition({ 1, 0, 2 });
    mesh3.SetPosition({ -1, 0, 2 });
    mesh4.SetPosition({ 0, 1, 2 });
    mesh5.SetPosition({ 0, -1, 2 });

    //mesh2.GetMaterial()->color = { 1, 0 ,0, 1 };
    //mesh3.GetMaterial()->color = { 0, 1 ,0, 1 };
    //mesh4.GetMaterial()->color = { 0, 0 ,1, 1 };
    //mesh5.GetMaterial()->color = { 1, 1 ,0, 1 };


    ComPtr<ID3D11Resource> resource;
    window.GetBackBuffer()->GetResource(&resource);

    ComPtr<ID3D11Texture2D> texture;
    resource.As(&texture);

    D3D11_TEXTURE2D_DESC textureDesc = {};
    texture->GetDesc(&textureDesc);

    textureDesc.MipLevels = 0;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depthStencilTexture;
    HRESULT hr = device.GetDevice()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create depth stencil texture", hr);
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = 0;
    depthStencilDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11DepthStencilView> depthStencilView;
    hr = device.GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &depthStencilView);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create depth stencil view", hr);
    }


    D3D11_DEPTH_STENCIL_DESC desc{};

    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    desc.StencilEnable = false;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace = desc.FrontFace;

    ComPtr<ID3D11DepthStencilState> depthStencilState;
    device.GetDevice()->CreateDepthStencilState(&desc, &depthStencilState);


    CameraData data(window.GetBackBuffer(), depthStencilView, depthStencilState);
    data.clipPlane.Near = 0.0001f;
    data.clipPlane.Far = 1000.f;
    //camera = renderer.CreateCamera(data);
}
void TriangleRenderInput2(SDL_Event event)
{
    switch(event.type)
    {
    case SDL_EventType::SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
        case SDL_KeyCode::SDLK_a:
            aPressed = true;
            break;
        case SDL_KeyCode::SDLK_w:
            wPressed = true;
            break;
        case SDL_KeyCode::SDLK_s:
            sPressed = true;
            break;
        case SDL_KeyCode::SDLK_d:
            dPressed = true;
            break;
        case SDL_KeyCode::SDLK_UP:
            upPressed = true;
            break;
        case SDL_KeyCode::SDLK_DOWN:
            downPressed = true;
            break;
        case SDL_KeyCode::SDLK_LEFT:
            leftPressed = true;
            break;
        case SDL_KeyCode::SDLK_RIGHT:
            rightPressed = true;
            break;
        case SDL_KeyCode::SDLK_LCTRL:
            ctrlPressed = true;
            break;
        }

        break;
    case SDL_EventType::SDL_KEYUP:
        switch(event.key.keysym.sym)
        {
        case SDL_KeyCode::SDLK_a:
            aPressed = false;
            break;
        case SDL_KeyCode::SDLK_w:
            wPressed = false;
            break;
        case SDL_KeyCode::SDLK_s:
            sPressed = false;
            break;
        case SDL_KeyCode::SDLK_d:
            dPressed = false;
            break;
        case SDL_KeyCode::SDLK_UP:
            upPressed = false;
            break;
        case SDL_KeyCode::SDLK_DOWN:
            downPressed = false;
            break;
        case SDL_KeyCode::SDLK_LEFT:
            leftPressed = false;
            break;
        case SDL_KeyCode::SDLK_RIGHT:
            rightPressed = false;
            break;
        case SDL_KeyCode::SDLK_LCTRL:
            ctrlPressed = false;
            break;
        }
        break;
    }
}
void TriangleRenderUpdate2(float dt)
{

    Vector2f axis;
    Vector3f cameraDirection;
    Vector3f cameraRotation;
    if(aPressed)
    {
        axis.y() -= 1;
    }
    if(sPressed)
    {
        axis.x() -= 1;
    }
    if(wPressed)
    {
        axis.x() += 1;
    }
    if(dPressed)
    {
        axis.y() += 1;
    }

    //if(!ctrlPressed)
    //{
    //    if(upPressed)
    //    {
    //        cameraDirection.z() += 1;
    //    }
    //    if(downPressed)
    //    {
    //        cameraDirection.z() -= 1;
    //    }
    //    if(leftPressed)
    //    {
    //        cameraDirection.x() -= 1;
    //    }
    //    if(rightPressed)
    //    {
    //        cameraDirection.x() += 1;
    //    }
    //}
    //else
    //{
    //    if(upPressed)
    //    {
    //        cameraRotation.x() += 1;
    //    }
    //    if(downPressed)
    //    {
    //        cameraRotation.x() -= 1;
    //    }
    //    if(leftPressed)
    //    {
    //        cameraRotation.y() -= 1;
    //    }
    //    if(rightPressed)
    //    {
    //        cameraRotation.y() += 1;
    //    }
    //}

    //camera.SetPosition(camera.GetPosition() + cameraDirection * 20.f * dt);
    //camera.SetRotation(camera.GetRotation() * Quaternion<float>(cameraRotation, Degrees(20.f * dt)));

    mesh.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh2.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh3.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh4.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh5.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));

    Component<DX11::StaticMesh::MeshObject> test{ std::move(mesh) };
    mesh = std::move(test);

    static float accumulatedTime = 0;


    //mesh2.GetMaterial()->SetColor(Vector4f{ 1, 0, 0, 1 } *Vector4f{ Scalar<float>((cos(accumulatedTime) + 1) / 2) });
    //mesh3.GetMaterial()->SetColor(Vector4f{ 0, 1, 0, 1 } *Vector4f{ Scalar<float>((sin(accumulatedTime) + 1) / 2) });
    //mesh4.GetMaterial()->SetColor(Vector4f{ 0, 0, 1, 1 } *Vector4f{ Scalar<float>((sin(accumulatedTime) + 1) / 2) });
    //mesh5.GetMaterial()->SetColor(Vector4f{ 1, 1, 0, 1 } *Vector4f{ Scalar<float>((cos(accumulatedTime) + 1) / 2) });

    mesh.SetScale((Vector3f{ 1, 1, 1 } *Vector3f{ Scalar<float>((cos(accumulatedTime) + 1) / 2) }));
    accumulatedTime += dt;
}

void TriangleRenderShutdown2()
{
    mesh = nullptr;
    mesh2 = nullptr;
    mesh3 = nullptr;
    mesh4 = nullptr;
    mesh5 = nullptr;
    //camera = nullptr;
}
