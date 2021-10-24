#include "Renderer.h"
#include "../DX11/Device.h"
#include "../DX11/Helpers.h"
#include "../DX11/CommonInclude.h"
#include "../DX11/InputLayouts.h"
#include "../DX11/ShaderConstants.h"
#include "Extensions/MatrixExtension.h"
#include <assert.h>
#include <algorithm>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine::DX11;



static void SetMaterial(const InsanityEngine::DX11::Device& device, const StaticMesh::Material& mat);
static void SetMesh(const InsanityEngine::DX11::Device& device, const StaticMesh::Mesh& mesh);
static void DrawMesh(const InsanityEngine::DX11::Device& device, const Application::MeshObject& mesh);

namespace InsanityEngine::Application
{
    Renderer::Renderer(DX11::Device& device) :
        m_device(&device)
    {
        m_inputLayouts[positionNormalUVLayout] = DX11::InputLayouts::PositionNormalUV::CreateInputLayout(m_device->GetDevice());
    }

    CameraHandle Renderer::CreateCamera(ComPtr<ID3D11RenderTargetView> renderTarget, ComPtr<ID3D11DepthStencilView> depthStencil, ComPtr<ID3D11DepthStencilState> depthStencilState)
    {
        assert(renderTarget != nullptr);
        Engine::Camera camera{ renderTarget, depthStencil, depthStencilState };
        StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera.GetViewMatrix(), camera.GetPerspectiveMatrix()) };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);
        m_cameras.push_back(std::make_unique<CameraObject>(constantBuffer, std::move(camera)));
        return CameraHandle(*this, *m_cameras.back().get());
    }

    MeshHandle Renderer::CreateMesh(std::shared_ptr<DX11::StaticMesh::Mesh> mesh, std::shared_ptr<DX11::StaticMesh::Material> material)
    {
        StaticMesh::MeshObject obj{ mesh, material };
        StaticMesh::Constants::VSMesh constants{ .worldMatrix = obj.GetObjectMatrix() };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);

        m_meshes.push_back(std::make_unique<MeshObject>(constantBuffer, std::move(obj)));
        return MeshHandle(*this, *m_meshes.back().get());
    }

    void Renderer::Draw(ComPtr<ID3D11RenderTargetView1> backBuffer)
    {
        DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), backBuffer.Get(), Vector4f{ 0, 0.3f, 0.7f, 1 });

        //Updates all mesh constant buffers
        for(const auto& mesh : m_meshes)
        {
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::VSMesh constants{ .worldMatrix = mesh->mesh.GetObjectMatrix() };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(mesh->GetConstantBuffer(), 0);
        }

        for(const auto& camera : m_cameras)
        {
            //Update camera constant buffers
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(camera->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera->camera.GetViewMatrix(), camera->camera.GetPerspectiveMatrix()) };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(camera->GetConstantBuffer(), 0);

            //Clear render targets
            if(camera->camera.GetRenderTargetView() != backBuffer.Get())
                DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), camera->camera.GetRenderTargetView(), Vector4f{ 0, 0.3f, 0.7f, 1 });

            if(camera->camera.GetDepthStencilView() != nullptr)
                m_device->GetDeviceContext()->ClearDepthStencilView(camera->camera.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

            //Setting the cameras
            Vector2f resolution = camera->camera.GetRenderTargetResolution();

            D3D11_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(resolution.x());
            viewport.Height = static_cast<float>(resolution.y());
            viewport.MaxDepth = 1;
            viewport.MinDepth = 0;

            D3D11_RECT rect = {};
            rect.right = static_cast<LONG>(resolution.x());
            rect.bottom = static_cast<LONG>(resolution.y());


            std::array renderTargets { static_cast<ID3D11RenderTargetView*>(camera->camera.GetRenderTargetView())};
            m_device->GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera->camera.GetDepthStencilView());
            m_device->GetDeviceContext()->OMSetDepthStencilState(camera->camera.GetDepthStencilState(), 0);
            m_device->GetDeviceContext()->RSSetViewports(1, &viewport);
            m_device->GetDeviceContext()->RSSetScissorRects(1, &rect);

            //Mesh rendering set up
            std::array vsCameraBuffer { camera->GetConstantBuffer() };
            m_device->GetDeviceContext()->VSSetConstantBuffers(StaticMesh::Registers::VS::cameraConstants, static_cast<UINT>(vsCameraBuffer.size()), vsCameraBuffer.data());

            m_device->GetDeviceContext()->IASetInputLayout(m_inputLayouts[positionNormalUVLayout].Get());
            m_device->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //Draw meshes
            for(const auto& mesh : m_meshes)
            {
                SetMaterial(*m_device, *mesh->mesh.GetMaterial());
                SetMesh(*m_device, *mesh->mesh.GetMesh());
                DrawMesh(*m_device, *mesh);
            }
        }
    }

    void Renderer::Destroy(CameraObject* object)
    {
        auto it = std::find_if(m_cameras.begin(), m_cameras.end(), [=](std::unique_ptr<CameraObject>& o) { return o.get() == object; });
        if(it != m_cameras.end())
            m_cameras.erase(it, m_cameras.end());
    }

    void Renderer::Destroy(MeshObject* object)
    {
        auto it = std::find_if(m_meshes.begin(), m_meshes.end(), [=](std::unique_ptr<MeshObject>& o) { return o.get() == object; });

        if(it != m_meshes.end())
            m_meshes.erase(it, m_meshes.end());
    }

    CameraObject::CameraObject(ComPtr<ID3D11Buffer> cameraConstants, InsanityEngine::Engine::Camera&& camera) :
        m_cameraConstants(cameraConstants),
        camera(std::move(camera))
    {
        assert(m_cameraConstants != nullptr);
    }

    MeshObject::MeshObject(ComPtr<ID3D11Buffer> constantBuffer, DX11::StaticMesh::MeshObject&& mesh) :
        m_objectConstants(constantBuffer),
        mesh(std::move(mesh))
    {
        assert(m_objectConstants != nullptr);
    }
}


void SetMaterial(const InsanityEngine::DX11::Device& device, const StaticMesh::Material& mat)
{
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->GetView() };
    std::array constantBuffers{ mat.GetConstantBuffer()};
    device.GetDeviceContext()->PSSetSamplers        (StaticMesh::Registers::PS::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources (StaticMesh::Registers::PS::albedoTexture, 1, textures.data());
    device.GetDeviceContext()->PSSetConstantBuffers (StaticMesh::Registers::PS::materialConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
}

void SetMesh(const InsanityEngine::DX11::Device& device, const StaticMesh::Mesh& mesh)
{
    std::array vertexBuffers { mesh.GetVertexBuffer() };

    UINT stride = sizeof(StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(const InsanityEngine::DX11::Device& device, const Application::MeshObject& mesh)
{
    std::array constantBuffers { mesh.GetConstantBuffer() };
    device.GetDeviceContext()->VSSetConstantBuffers(StaticMesh::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
    device.GetDeviceContext()->DrawIndexed(mesh.mesh.GetMesh()->GetIndexCount(), 0, 0);
}