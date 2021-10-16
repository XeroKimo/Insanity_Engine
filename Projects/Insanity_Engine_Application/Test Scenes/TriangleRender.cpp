#include "TriangleRender.h"

#include "Extensions/MatrixExtension.h"
#include "../Application/Application.h"
#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../DX11/Mesh.h"
#include "../DX11/Helpers.h"
#include "../DX11/InputLayouts.h"
#include "../DX11/Renderers.h"

#include "Debug Classes/Exceptions/HRESULTException.h"
#include "../Engine/Camera.h"

#include "SDL.h"
#include <optional>


using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Debug::Exceptions;
using namespace InsanityEngine::Math::Types;
using InsanityEngine::DX11::ComPtr;

static ComPtr<ID3D11VertexShader> vertexShader;
static ComPtr<ID3D11PixelShader> pixelShader;
static ComPtr<ID3D11InputLayout> inputLayout;
static ComPtr<ID3D11Buffer> cameraBuffer;
static ComPtr<ID3D11DepthStencilState> depthStencilState;

static std::shared_ptr<DX11::StaticMesh::Texture> texture;
static std::shared_ptr<DX11::StaticMesh::Material> material;
static std::shared_ptr<DX11::StaticMesh::Shader> shader;
static std::shared_ptr<DX11::StaticMesh::Mesh> mesh;

static std::optional<DX11::Renderers::StaticMesh::Renderer> g_renderer;
static DX11::Renderers::StaticMesh::RenderObject* g_renderObject;
static std::optional<InsanityEngine::Engine::Camera> camera;

static ComPtr<ID3D11Buffer> colorBufferTest;
static ComPtr<ID3D11ShaderResourceView> textureView;
static ComPtr<ID3D11SamplerState> samplerState;

static bool aPressed = false;
static bool wPressed = false;
static bool sPressed = false;
static bool dPressed = false;

void InitializeShaders(InsanityEngine::DX11::Device& device);
void InitializeMaterial(InsanityEngine::DX11::Device& device);
void InitializeMesh(InsanityEngine::DX11::Device& device);
void InitializeCamera(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window);

void SetMaterial(InsanityEngine::DX11::Device& device, const StaticMesh::Material& mat);
void SetMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh);
void DrawMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh);

void TriangleRenderSetup(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
    g_renderer = Renderers::StaticMesh::Renderer(device);
    InitializeShaders(device);
    InitializeMaterial(device);
    InitializeMesh(device);
    InitializeCamera(device, window);

}

void TriangleRenderInput(SDL_Event event)
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

void TriangleRenderUpdate(float dt)
{
    //meshObject->quat *= Quaternion<float>(Degrees<float>(), Degrees<float>(), Degrees<float>(90.f * dt));

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

    g_renderObject->object.quat *= Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt));
}

void TriangleRender(DX11::Device& device, InsanityEngine::Application::Window& window)
{
    D3D11_MAPPED_SUBRESOURCE subresource;
    device.GetDeviceContext()->Map(g_renderObject->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

    DX11::Renderers::StaticMesh::VSObjectConstants constants{ .worldMatrix = g_renderObject->object.GetObjectMatrix() };
    std::memcpy(subresource.pData, &constants, sizeof(constants));
    device.GetDeviceContext()->Unmap(g_renderObject->GetConstantBuffer(), 0);


    std::array renderTargets
    {
        static_cast<ID3D11RenderTargetView*>(window.GetBackBuffer())
    };

    Vector2f resolution = window.GetWindowSize();

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(resolution.x());
    viewport.Height = static_cast<float>(resolution.y());
    viewport.MaxDepth = 1;
    viewport.MinDepth = 0;

    D3D11_RECT rect = {};
    rect.right = static_cast<LONG>(resolution.x());
    rect.bottom = static_cast<LONG>(resolution.y());

    DX11::Helpers::ClearRenderTargetView(device.GetDeviceContext(), camera->GetRenderTargetView(), Vector4f{ 0, 0.3f, 0.7f, 1 });
    device.GetDeviceContext()->ClearDepthStencilView(camera->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    device.GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera->GetDepthStencilView());
    device.GetDeviceContext()->OMSetDepthStencilState(depthStencilState.Get(), 0);
    device.GetDeviceContext()->RSSetViewports(1, &viewport);
    device.GetDeviceContext()->RSSetScissorRects(1, &rect);
    device.GetDeviceContext()->IASetInputLayout(inputLayout.Get());

    SetMaterial(device, *g_renderObject->object.GetMaterial().get());

    std::array vsConstantBuffers
    {
        cameraBuffer.Get(),
        g_renderObject->GetConstantBuffer()
    };

    device.GetDeviceContext()->VSSetConstantBuffers(Renderers::Registers::VS::Shared::cameraConstants, vsConstantBuffers.size(), vsConstantBuffers.data());
    device.GetDeviceContext()->PSSetConstantBuffers(Renderers::Registers::PS::StaticMesh::materialConstants, 1, colorBufferTest.GetAddressOf());

    SetMesh(device, g_renderObject->object);
    DrawMesh(device, g_renderObject->object);
}

void ShutdownTriangleRender()
{
    //DirectX::SetWICFactory(nullptr);

    g_renderer->DestroyObject(g_renderObject);
    vertexShader = nullptr;
    pixelShader = nullptr;
    inputLayout = nullptr;
    cameraBuffer = nullptr;
    depthStencilState = nullptr;
    colorBufferTest = nullptr;
}

void InitializeShaders(InsanityEngine::DX11::Device& device)
{
    ComPtr<ID3DBlob> data;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3DCompileFromFile(
        L"Resources/Shaders/VertexShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "vs_5_0",
        0,
        0,
        &data,
        &error);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to compile vertex shader", hr);
    }

    hr = device.GetDevice()->CreateVertexShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &vertexShader);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create vertex shader", hr);
    }

    inputLayout = InputLayouts::PositionNormalUV::CreateInputLayout(device.GetDevice());

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create input layout", hr);
    }

    hr = D3DCompileFromFile(
        L"Resources/Shaders/PixelShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "ps_5_0",
        0,
        0,
        &data,
        &error);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to compile pixel shader", hr);
    }

    hr = device.GetDevice()->CreatePixelShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &pixelShader);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create pixel shader", hr);
    }

    shader = std::make_shared<StaticMesh::Shader>(vertexShader, pixelShader);
}

void InitializeMaterial(InsanityEngine::DX11::Device& device)
{
    HRESULT hr = DX11::Helpers::CreateTextureFromFile(device.GetDevice(), &textureView, L"Resources/Korone_NotLikeThis.png", DirectX::WIC_FLAGS_NONE);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create texture", hr);
    }

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

    hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &samplerState);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create sampler state", hr);
    }

    texture = std::make_shared<StaticMesh::Texture>(textureView, samplerState);
    material = std::make_shared<StaticMesh::Material>(shader, texture);
}

void InitializeMesh(InsanityEngine::DX11::Device& device)
{
    auto vertices = std::to_array(
        {
            StaticMesh::VertexData{ Vector3f(-0.5f, -0.5f, 0), Vector3f(), Vector2f(0, 0) },
            StaticMesh::VertexData{ Vector3f(0, 0.5f, 0), Vector3f(), Vector2f(0.5f, 1) },
            StaticMesh::VertexData{ Vector3f(0.5f, -0.5f, 0), Vector3f(), Vector2f(1, 0) }
        }
    );
    ComPtr<ID3D11Buffer> vertexBuffer;
    Helpers::CreateVertexBuffer(device.GetDevice(), &vertexBuffer, std::span(vertices));

    auto indices = std::to_array<UINT>(
        {
            0, 1, 2
        }
    );

    ComPtr<ID3D11Buffer> indexBuffer;
    Helpers::CreateIndexBuffer(device.GetDevice(), &indexBuffer, std::span(indices));
    mesh = std::make_shared<StaticMesh::Mesh>(vertexBuffer, static_cast<UINT>(vertices.size()), indexBuffer, static_cast<UINT>(indices.size()));


    g_renderObject = g_renderer->CreateObject(mesh, material);
    g_renderObject->object.position.z() = 2;

    Helpers::CreateConstantBuffer(device.GetDevice(), &colorBufferTest, true, g_renderObject->object.GetMaterial()->color);
}

void InitializeCamera(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
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

    camera = Engine::Camera(ComPtr<ID3D11RenderTargetView>(window.GetBackBuffer()), depthStencilView);

    Vector2f windowSize = window.GetWindowSize();
    //camera->position.x() = 3;
    Matrix4x4f viewProjection = Math::Matrix::PerspectiveProjectionLH(Degrees<float>(90), windowSize.x() / windowSize.y(), camera->clipPlane.Near, camera->clipPlane.Far) * camera->GetViewMatrix();

    Helpers::CreateConstantBuffer(device.GetDevice(), &cameraBuffer, true, viewProjection);

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

    device.GetDevice()->CreateDepthStencilState(&desc, &depthStencilState);
}


void SetMaterial(InsanityEngine::DX11::Device& device, const StaticMesh::Material& mat)
{
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->GetView() };
    device.GetDeviceContext()->PSSetSamplers(Renderers::Registers::PS::StaticMesh::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources(Renderers::Registers::PS::StaticMesh::albedoTexture, 1, textures.data());
}

void SetMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh)
{
    std::array vertexBuffers
    {
        mesh.GetMesh()->GetVertexBuffer()
    };

    UINT stride = sizeof(StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetMesh()->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh)
{
    device.GetDeviceContext()->DrawIndexed(mesh.GetMesh()->GetIndexCount(), 0, 0);
}
