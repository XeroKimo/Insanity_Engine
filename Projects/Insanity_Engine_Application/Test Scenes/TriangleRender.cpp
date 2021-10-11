#include "TriangleRender.h"

#include "Extensions/MatrixExtension.h"
#include "../Application/Application.h"
#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../DX11/Mesh.h"
#include "../DX11/Helpers.h"
#include "../DX11/ResourceRegistry.h"

#include "Debug Classes/Exceptions/HRESULTException.h"
#include "../Engine/Camera.h"

#include "SDL.h"
#include <optional>


using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Debug::Exceptions;
using namespace InsanityEngine::Math::Types;
using InsanityEngine::DX11::ComPtr;

static DX11::ResourceRegistry registry;

static constexpr std::string_view g_VertexShader = "Vertex Shader";
static constexpr std::string_view g_PixelShader = "Pixel Shader";
static constexpr std::string_view g_InputLayout = "Input Layout";
static constexpr std::string_view g_DepthStencilState = "Depth Stencil State";
static constexpr std::string_view g_TextureView = "Texture View";
static constexpr std::string_view g_SamplerState = "Sampler State";

static ComPtr<ID3D11Buffer> cameraBuffer;

static DX11::StaticMesh::ResourceManager resourceManager;
static constexpr std::string_view g_Texture = "Texture";
static constexpr std::string_view g_Material = "Material";
static constexpr std::string_view g_Shader = "Shader";
static constexpr std::string_view g_Mesh = "Mesh";

static std::optional<DX11::StaticMesh::MeshObject> meshObject;
static std::optional<Engine::Camera> camera;
static ComPtr<ID3D11Buffer> meshObjectBuffer;

static ComPtr<ID3D11Buffer> colorBufferTest;

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

    meshObject->quat *= Quaternion<float>(Vector3f(axis, 0), Degrees<float>(90.f * dt));
}

void TriangleRender(DX11::Device& device, InsanityEngine::Application::Window& window)
{
    D3D11_MAPPED_SUBRESOURCE subresource;
    device.GetDeviceContext()->Map(meshObjectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    Matrix4x4f f = meshObject->GetObjectMatrix();
    std::memcpy(subresource.pData, &f, sizeof(f));
    device.GetDeviceContext()->Unmap(meshObjectBuffer.Get(), 0);


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
    device.GetDeviceContext()->OMSetDepthStencilState(registry.Get<ID3D11DepthStencilState>(g_DepthStencilState).Get(), 0);
    device.GetDeviceContext()->RSSetViewports(1, &viewport);
    device.GetDeviceContext()->RSSetScissorRects(1, &rect);
    device.GetDeviceContext()->IASetInputLayout(registry.Get<ID3D11InputLayout>(g_InputLayout).Get());

    SetMaterial(device, meshObject->GetMaterial()->Get());

    auto vsConstantBuffers = std::to_array(
        {
            cameraBuffer.Get(),
            meshObjectBuffer.Get()
        });

    device.GetDeviceContext()->VSSetConstantBuffers(1, 2, vsConstantBuffers.data());
    device.GetDeviceContext()->PSSetConstantBuffers(1, 1, colorBufferTest.GetAddressOf());

    SetMesh(device, meshObject.value());
    DrawMesh(device, meshObject.value());
}

void ShutdownTriangleRender()
{
    //DirectX::SetWICFactory(nullptr);
    cameraBuffer = nullptr;
    meshObjectBuffer = nullptr;
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

    ComPtr<ID3D11VertexShader> vertexShader;
    hr = device.GetDevice()->CreateVertexShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &vertexShader);
    registry.Register(g_VertexShader, vertexShader);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create vertex shader", hr);
    }

    auto elements = StaticMesh::GetInputElementDescription();
    ComPtr<ID3D11InputLayout> inputLayout;
    device.GetDevice()->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), data->GetBufferPointer(), data->GetBufferSize(), &inputLayout);

    registry.Register(g_InputLayout, inputLayout);

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

    ComPtr<ID3D11PixelShader> pixelShader;

    hr = device.GetDevice()->CreatePixelShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &pixelShader);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create pixel shader", hr);
    }

    registry.Register(g_PixelShader, pixelShader);

    resourceManager.CreateShader(g_Shader, StaticMesh::Shader(vertexShader, pixelShader));
}

void InitializeMaterial(InsanityEngine::DX11::Device& device)
{
    ComPtr<ID3D11ShaderResourceView> textureView;
    HRESULT hr = DX11::Helpers::CreateTextureFromFile(device.GetDevice(), &textureView, L"Resources/Korone_NotLikeThis.png", DirectX::WIC_FLAGS_NONE);
    registry.Register(g_TextureView, textureView);

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

    ComPtr<ID3D11SamplerState> samplerState;
    hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &samplerState);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create sampler state", hr);
    }
    registry.Register(g_SamplerState, samplerState);

    StaticMesh::SharedResource<StaticMesh::Texture> texture = resourceManager.CreateTexture(g_Texture, StaticMesh::Texture(textureView, samplerState));
    resourceManager.CreateMaterial(g_Material, StaticMesh::Material(resourceManager.GetShader(g_Shader), texture));
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

    StaticMesh::SharedResource<StaticMesh::Mesh> mesh = resourceManager.CreateMesh(g_Mesh, StaticMesh::Mesh(vertexBuffer, static_cast<UINT>(vertices.size()), indexBuffer, static_cast<UINT>(indices.size())));

    meshObject = DX11::StaticMesh::MeshObject(mesh, resourceManager.GetMaterial(g_Material));

    meshObject->position.z() = 2;
    Matrix4x4f objectMatrix = meshObject->GetObjectMatrix();

    Helpers::CreateConstantBuffer(device.GetDevice(), &meshObjectBuffer, objectMatrix, true);

    Helpers::CreateConstantBuffer(device.GetDevice(), &colorBufferTest, meshObject->GetMaterial()->Get().color, true);
}

void InitializeCamera(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
    camera = Engine::Camera(ComPtr<ID3D11Device5>(device.GetDevice()), ComPtr<ID3D11RenderTargetView>(window.GetBackBuffer()), true);

    Vector2f windowSize = window.GetWindowSize();
    //camera->position.x() = 3;
    Matrix4x4f viewProjection = Math::Matrix::PerspectiveProjectionLH(Degrees<float>(90), windowSize.x() / windowSize.y(), camera->clipPlane.Near, camera->clipPlane.Far) * camera->GetViewMatrix();

    Helpers::CreateConstantBuffer(device.GetDevice(), &cameraBuffer, viewProjection, true);

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
    registry.Register(g_DepthStencilState, depthStencilState);
}


void SetMaterial(InsanityEngine::DX11::Device& device, const StaticMesh::Material& mat)
{
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->Get().GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->Get().GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->Get().GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->Get().GetView() };
    device.GetDeviceContext()->PSSetSamplers(0, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources(0, 1, textures.data());
}

void SetMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh)
{
    std::array vertexBuffers
    {
        mesh.GetMesh()->Get().GetVertexBuffer()
    };

    UINT stride = sizeof(StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetMesh()->Get().GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(InsanityEngine::DX11::Device& device, const StaticMesh::MeshObject& mesh)
{
    device.GetDeviceContext()->DrawIndexed(mesh.GetMesh()->Get().GetIndexCount(), 0, 0);
}
