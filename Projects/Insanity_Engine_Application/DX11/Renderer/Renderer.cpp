#include "Renderer.h"
#include "../Device.h"
#include "../Helpers.h"
#include "../CommonInclude.h"
#include "../InputLayouts.h"
#include "../ShaderConstants.h"
#include "Extensions/MatrixExtension.h"
#include <assert.h>
#include <algorithm>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;



static void SetMaterial(const InsanityEngine::DX11::Device& device, const DX11::Resources::StaticMesh::Material& mat);
static void SetMesh(const InsanityEngine::DX11::Device& device, const DX11::Resources::Mesh& mesh);
static void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::MeshObject& mesh);

namespace InsanityEngine::DX11
{
    Renderer::Renderer(DX11::Device& device) :
        m_device(&device)
    {
        m_inputLayouts[positionNormalUVLayout] = DX11::InputLayouts::PositionNormalUV::CreateInputLayout(m_device->GetDevice());
    }

    CameraHandle Renderer::CreateCamera(CameraData data)
    {
        StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(data.GetViewMatrix(), data.GetPerspectiveMatrix()) };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);
        m_cameras.push_back(std::make_unique<CameraObject>(constantBuffer, std::move(data)));
        return CameraHandle(*this, *m_cameras.back().get());
    }

    StaticMeshHandle Renderer::CreateMesh(DX11::StaticMesh::MeshObjectData data)
    {
        StaticMesh::Constants::VSMesh constants{ .worldMatrix = data.GetObjectMatrix() };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);

        m_meshes.push_back(std::make_unique<StaticMesh::MeshObject>(constantBuffer, std::move(data)));
        return StaticMeshHandle(*this, *m_meshes.back().get());
    }

    void Renderer::Draw(ComPtr<ID3D11RenderTargetView1> backBuffer)
    {
        DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), backBuffer.Get(), Vector4f{ 0, 0.3f, 0.7f, 1 });

        //Updates all mesh and materials (might dupe for now but who cares) constant buffers
        for(const auto& mesh : m_meshes)
        {
            //Mesh constants updated
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::VSMesh constants{ .worldMatrix = mesh->data.GetObjectMatrix() };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(mesh->GetConstantBuffer(), 0);

            //Material constants updated
            m_device->GetDeviceContext()->Map(mesh->data.GetMaterial()->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::PSMaterial psConstants{ .color = mesh->data.GetMaterial()->GetColor() };
            std::memcpy(subresource.pData, &psConstants, sizeof(psConstants));
            m_device->GetDeviceContext()->Unmap(mesh->data.GetMaterial()->GetConstantBuffer(), 0);
        }

        for(const auto& camera : m_cameras)
        {
            //Update camera constant buffers
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(camera->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera->data.GetViewMatrix(), camera->data.GetPerspectiveMatrix()) };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(camera->GetConstantBuffer(), 0);

            //Clear render targets
            if(camera->data.GetRenderTargetView() != backBuffer.Get())
                DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), camera->data.GetRenderTargetView(), Vector4f{ 0, 0.3f, 0.7f, 1 });

            if(camera->data.GetDepthStencilView() != nullptr)
                m_device->GetDeviceContext()->ClearDepthStencilView(camera->data.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

            //Setting the cameras
            Vector2f resolution = camera->data.GetRenderTargetResolution();

            D3D11_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(resolution.x());
            viewport.Height = static_cast<float>(resolution.y());
            viewport.MaxDepth = 1;
            viewport.MinDepth = 0;

            D3D11_RECT rect = {};
            rect.right = static_cast<LONG>(resolution.x());
            rect.bottom = static_cast<LONG>(resolution.y());


            std::array renderTargets { static_cast<ID3D11RenderTargetView*>(camera->data.GetRenderTargetView())};
            m_device->GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera->data.GetDepthStencilView());
            m_device->GetDeviceContext()->OMSetDepthStencilState(camera->data.GetDepthStencilState(), 0);
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
                SetMaterial(*m_device, *mesh->data.GetMaterial());
                SetMesh(*m_device, *mesh->data.GetMesh());
                DrawMesh(*m_device, *mesh);
            }
        }
    }

    void Renderer::Destroy(CameraObject* object)
    {
        auto it = std::remove_if(m_cameras.begin(), m_cameras.end(), [=](std::unique_ptr<CameraObject>& o) { return o.get() == object; });
        if(it != m_cameras.end())
            m_cameras.erase(it, m_cameras.end());
    }

    void Renderer::Destroy(StaticMesh::MeshObject* object)
    {
        auto it = std::remove_if(m_meshes.begin(), m_meshes.end(), [=](std::unique_ptr<StaticMesh::MeshObject>& o) { return o.get() == object; });

        if(it != m_meshes.end())
            m_meshes.erase(it, m_meshes.end());
    }

}


void SetMaterial(const InsanityEngine::DX11::Device& device, const DX11::Resources::StaticMesh::Material& mat)
{
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->GetView() };
    std::array constantBuffers{ mat.GetConstantBuffer()};
    device.GetDeviceContext()->PSSetSamplers        (DX11::StaticMesh::Registers::PS::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources (DX11::StaticMesh::Registers::PS::albedoTexture, 1, textures.data());
    device.GetDeviceContext()->PSSetConstantBuffers (DX11::StaticMesh::Registers::PS::materialConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
}

void SetMesh(const InsanityEngine::DX11::Device& device, const DX11::Resources::Mesh& mesh)
{
    std::array vertexBuffers { mesh.GetVertexBuffer() };

    UINT stride = sizeof(DX11::StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::MeshObject& mesh)
{
    std::array constantBuffers { mesh.GetConstantBuffer() };
    device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
    device.GetDeviceContext()->DrawIndexed(mesh.data.GetMesh()->GetIndexCount(), 0, 0);
}