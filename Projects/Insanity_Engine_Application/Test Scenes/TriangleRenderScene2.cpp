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

static std::optional<Application::MeshHandle> mesh;
static std::optional<Application::CameraHandle> camera;

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
    mesh = renderer.CreateMesh(nullptr, nullptr);
    mesh->SetPosition({ 0, 0, 3 });

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

}
void TriangleRenderUpdate2(float dt)
{

}