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

namespace InsanityEngine::DX11::StaticMesh
{
    Renderer::Renderer(DX11::Device& device) :
        m_device(&device)
    {
        m_inputLayouts[positionNormalUVLayout] = DX11::InputLayouts::PositionNormalUV::CreateInputLayout(m_device->GetDevice());
    }

    StaticMeshHandle Renderer::CreateMesh(MeshObjectData data)
    {
        StaticMesh::Constants::VSMesh constants{ .worldMatrix = data.GetObjectMatrix() };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);

        m_meshes.push_back(std::make_unique<StaticMesh::MeshObject>(constantBuffer, std::move(data)));
        return StaticMeshHandle(*this, *m_meshes.back().get());
    }

    void Renderer::Update()
    {
        for(const auto& mesh : m_meshes)
        {
            //Mesh constants updated
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::VSMesh constants{ .worldMatrix = mesh->data.GetObjectMatrix() };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(mesh->GetConstantBuffer(), 0);

            //Material constants updated
            m_device->GetDeviceContext()->Map(mesh->data.GetMaterial()->Get().GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::PSMaterial psConstants{ .color = mesh->data.GetMaterial()->Get().GetColor() };
            std::memcpy(subresource.pData, &psConstants, sizeof(psConstants));
            m_device->GetDeviceContext()->Unmap(mesh->data.GetMaterial()->Get().GetConstantBuffer(), 0);
        }
    }

    void Renderer::Draw()
    {
        m_device->GetDeviceContext()->IASetInputLayout(m_inputLayouts[positionNormalUVLayout].Get());
        m_device->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //Draw meshes
        for(const auto& mesh : m_meshes)
        {
            SetMaterial(*m_device, mesh->data.GetMaterial()->Get());
            SetMesh(*m_device, mesh->data.GetMesh()->Get());
            DrawMesh(*m_device, *mesh);
        }

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
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->Get().GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->Get().GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->Get().GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->Get().GetView() };
    std::array constantBuffers{ mat.GetConstantBuffer() };
    device.GetDeviceContext()->PSSetSamplers(DX11::StaticMesh::Registers::PS::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources(DX11::StaticMesh::Registers::PS::albedoTexture, 1, textures.data());
    device.GetDeviceContext()->PSSetConstantBuffers(DX11::StaticMesh::Registers::PS::materialConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
}

void SetMesh(const InsanityEngine::DX11::Device& device, const DX11::Resources::Mesh& mesh)
{
    std::array vertexBuffers{ mesh.GetVertexBuffer() };

    UINT stride = sizeof(DX11::StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::MeshObject& mesh)
{
    std::array constantBuffers{ mesh.GetConstantBuffer() };
    device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
    device.GetDeviceContext()->DrawIndexed(mesh.data.GetMesh()->Get().GetIndexCount(), 0, 0);
}