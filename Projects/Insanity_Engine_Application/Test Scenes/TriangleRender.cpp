#include "TriangleRender.h"

#include "Extensions/MatrixExtension.h"
#include "../Application/Application.h"
#include "../Application/Window.h"
#include "../DX11/Device.h"
#include "../DX11/Mesh.h"
#include "../DX11/Helpers.h"

#include "Debug Classes/Exceptions/HRESULTException.h"
#include "../Engine/Camera.h"

#include "DirectXTex/DirectXTex.h"
#include <optional>


using namespace InsanityEngine;
using namespace InsanityEngine::DX11;
using namespace InsanityEngine::Debug::Exceptions;
using namespace InsanityEngine::Math::Types;
using InsanityEngine::DX11::ComPtr;

ComPtr<ID3D11VertexShader> vertexShader;
ComPtr<ID3D11PixelShader> pixelShader;
ComPtr<ID3D11InputLayout> inputLayout;
std::optional<InsanityEngine::Engine::Camera> camera;
ComPtr<ID3D11Buffer> cameraBuffer;
ComPtr<ID3D11DepthStencilState> depthStencilState;

std::optional<InsanityEngine::DX11::StaticMesh::Mesh> mesh;
std::optional<InsanityEngine::DX11::StaticMesh::MeshObject> meshObject;
ComPtr<ID3D11Buffer> meshObjectBuffer;

ComPtr<ID3D11Buffer> colorBufferTest;
ComPtr<ID3D11ShaderResourceView> textureView;
ComPtr<ID3D11SamplerState> samplerState;

void LoadImageFromFile(InsanityEngine::DX11::Device& device)
{
    DirectX::TexMetadata metaData;
    DirectX::ScratchImage scratchImage;
    HRESULT hr = DirectX::LoadFromWICFile(L"Resources/Korone_NotLikeThis.png", DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &metaData, scratchImage);
    if(FAILED(hr))
    {
            throw HRESULTException("Failed to load image", hr);
    }

    const DirectX::Image* image = scratchImage.GetImage(0, 0, 0);
    DirectX::ScratchImage scratchImage2;
    hr = DirectX::FlipRotate(*image, DirectX::TEX_FR_FLAGS::TEX_FR_FLIP_VERTICAL, scratchImage2);

    if(FAILED(hr))
    {
        throw HRESULTException("Failed to flip image", hr);
    }

    hr = DirectX::CreateShaderResourceView(device.GetDevice(), scratchImage2.GetImage(0, 0, 0), 1, metaData, &textureView);
    if(FAILED(hr))
    {
        throw HRESULTException("Failed to create shader resource", hr);
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
}

void InitializeShaders(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
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

    auto elements = StaticMesh::GetInputElementDescription();
    device.GetDevice()->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), data->GetBufferPointer(), data->GetBufferSize(), &inputLayout);

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
}

void InitializeMesh(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
    auto vertices = std::to_array(
        {
            StaticMesh::VertexData{ Vector3f(-0.5f, -0.5f, 2), Vector3f(), Vector2f(0, 0) },
            StaticMesh::VertexData{ Vector3f(0, 0.5f, 2), Vector3f(), Vector2f(0.5f, 1) },
            StaticMesh::VertexData{ Vector3f(0.5f, -0.5f, 2), Vector3f(), Vector2f(1, 0) }
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
    mesh = StaticMesh::Mesh(vertexBuffer, static_cast<UINT>(vertices.size()), indexBuffer, static_cast<UINT>(indices.size()));


    meshObject = DX11::StaticMesh::MeshObject(mesh.value());

    meshObject->position.x() = 2;
    Matrix4x4f objectMatrix = meshObject->GetObjectMatrix();

    Helpers::CreateConstantBuffer(device.GetDevice(), &meshObjectBuffer, objectMatrix, true);

    Vector4f triangleColor{ 1, 0, 1, 1 };
    Helpers::CreateConstantBuffer(device.GetDevice(), &colorBufferTest, triangleColor, true);
}

void InitializeCamera(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
    camera = Engine::Camera(ComPtr<ID3D11Device5>(device.GetDevice()), ComPtr<ID3D11RenderTargetView>(window.GetBackBuffer()), true);

    Vector2f windowSize = window.GetWindowSize();
    camera->position.x() = 3;
    Matrix4x4f viewProjection = Math::Functions::Matrix::PerspectiveProjectionLH(Degrees<float>(90), windowSize.x() / windowSize.y(), camera->clipPlane.Near, camera->clipPlane.Far) * camera->GetViewMatrix();

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

    device.GetDevice()->CreateDepthStencilState(&desc, &depthStencilState);
}

void TriangleRenderSetup(InsanityEngine::DX11::Device& device, InsanityEngine::Application::Window& window)
{
    LoadImageFromFile(device);
   
    InitializeShaders(device, window);
    InitializeMesh(device, window);
    InitializeCamera(device, window);
}

void TriangleRender(DX11::Device& device, InsanityEngine::Application::Window& window)
{

    auto renderTargets = std::to_array<ID3D11RenderTargetView*>(
        {
            static_cast<ID3D11RenderTargetView*>(window.GetBackBuffer())
        });

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
    device.GetDeviceContext()->VSSetShader(vertexShader.Get(), nullptr, 0);

    auto vsConstantBuffers = std::to_array(
        {
            cameraBuffer.Get(),
            meshObjectBuffer.Get()
        });
    device.GetDeviceContext()->VSSetConstantBuffers(1, 2, vsConstantBuffers.data());
    device.GetDeviceContext()->PSSetShader(pixelShader.Get(), nullptr, 0);
    device.GetDeviceContext()->PSSetConstantBuffers(1, 1, colorBufferTest.GetAddressOf());
    device.GetDeviceContext()->PSSetShaderResources(0, 1, textureView.GetAddressOf());
    device.GetDeviceContext()->PSSetSamplers(0, 1, samplerState.GetAddressOf());


    auto vertexBuffers = std::to_array(
        {
            meshObject->mesh.GetVertexBuffer()
        });

    UINT stride = sizeof(StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(meshObject->mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    device.GetDeviceContext()->DrawIndexed(meshObject->mesh.GetIndexCount(), 0, 0);
}

void ShutdownTriangleRender()
{
    //DirectX::SetWICFactory(nullptr);
    vertexShader = nullptr;
    pixelShader = nullptr;
    inputLayout = nullptr;
    cameraBuffer = nullptr;
    depthStencilState = nullptr;
    meshObjectBuffer = nullptr;
    colorBufferTest = nullptr;
}
