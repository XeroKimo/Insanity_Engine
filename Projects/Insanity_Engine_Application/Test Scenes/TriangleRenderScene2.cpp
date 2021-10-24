#include "TriangleRenderScene2.h"
#include "../DX11/Device.h"
#include "../Application/Window.h"
#include "../Application/Renderer.h"

#include "Debug Classes/Exceptions/HRESULTException.h"

#include <optional>

using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Debug::Exceptions;
using namespace InsanityEngine::Math::Types;

static Application::MeshHandle mesh;
static Application::MeshHandle mesh2;
static Application::MeshHandle mesh3;
static Application::MeshHandle mesh4;
static Application::MeshHandle mesh5;
static Application::CameraHandle camera;

static std::shared_ptr<StaticMesh::Material> mat2;
static std::shared_ptr<StaticMesh::Material> mat3;
static std::shared_ptr<StaticMesh::Material> mat4;
static std::shared_ptr<StaticMesh::Material> mat5;

static bool aPressed = false;
static bool wPressed = false;
static bool sPressed = false;
static bool dPressed = false;

void TriangleRenderSetup2(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Renderer& renderer, InsanityEngine::Application::Window& window)
{

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD = FLT_MAX;

    ComPtr<ID3D11SamplerState> samplerState;
    HRESULT hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &samplerState);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create sampler state", hr);
    }


    StaticMesh::Material::defaultShader = StaticMesh::CreateShader(device.GetDevice(), L"Resources/Shaders/VertexShader.hlsl", L"Resources/Shaders/PixelShader.hlsl");
    StaticMesh::Material::defaultAlbedo = StaticMesh::CreateTexture(device.GetDevice(), L"Resources/Korone_NotLikeThis.png", samplerState);

    std::shared_ptr<StaticMesh::Texture> dank = StaticMesh::CreateTexture(device.GetDevice(), L"Resources/Dank.png", samplerState);
    auto vertices = std::to_array(
        {
            StaticMesh::VertexData{ Vector3f(-0.5f, -0.5f, 0), Vector3f(), Vector2f(0, 0) },
            StaticMesh::VertexData{ Vector3f(0, 0.5f, 0), Vector3f(), Vector2f(0.5f, 1) },
            StaticMesh::VertexData{ Vector3f(0.5f, -0.5f, 0), Vector3f(), Vector2f(1, 0) }
        });

    auto indices = std::to_array<UINT>(
        {
            0, 1, 2
        });

    StaticMesh::MeshObject::defaultMesh = StaticMesh::CreateMesh(device.GetDevice(), std::span(vertices), std::span(indices));
    StaticMesh::Material::defaultShader = StaticMesh::CreateShader(device.GetDevice(), L"Resources/Shaders/VertexShader.hlsl", L"Resources/Shaders/PixelShader.hlsl");
    StaticMesh::Material::defaultAlbedo = StaticMesh::CreateTexture(device.GetDevice(), L"Resources/Korone_NotLikeThis.png", samplerState);
    StaticMesh::MeshObject::defaultMaterial = StaticMesh::CreateMaterial(device.GetDevice(), nullptr, nullptr);
    mat2 = StaticMesh::CreateMaterial(device.GetDevice(), nullptr, dank, { 1, 0 ,0, 1 });
    mat3 = StaticMesh::CreateMaterial(device.GetDevice(), nullptr, dank, { 0, 1 ,0, 1 });
    mat4 = StaticMesh::CreateMaterial(device.GetDevice(), nullptr, dank, { 0, 0 ,1, 1 });
    mat5 = StaticMesh::CreateMaterial(device.GetDevice(), nullptr, dank, { 1, 1 ,0, 1 });


    mesh = renderer.CreateMesh(nullptr, nullptr);
    mesh2 = renderer.CreateMesh(nullptr, mat2);
    mesh3 = renderer.CreateMesh(nullptr, mat3);
    mesh4 = renderer.CreateMesh(nullptr, mat4);
    mesh5 = renderer.CreateMesh(nullptr, mat5);

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
    hr = device.GetDevice()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

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



    camera = renderer.CreateCamera(window.GetBackBuffer(), depthStencilView, depthStencilState);
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
        }
        break;
    }
}
void TriangleRenderUpdate2(float dt)
{

    Vector2f axis;
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

    mesh.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh2.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh3.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh4.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
    mesh5.Rotate(Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt)));
}

void TriangleRenderShutdown2()
{
    mesh = nullptr;
    mesh2 = nullptr;
    mesh3 = nullptr;
    mesh4 = nullptr;
    mesh5 = nullptr;
    camera = nullptr;
}
