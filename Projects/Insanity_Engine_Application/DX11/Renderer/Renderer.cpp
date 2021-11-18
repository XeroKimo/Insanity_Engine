#include "Renderer.h"
#include "../Device.h"
#include "../Helpers.h"
#include "../CommonInclude.h"
#include "../InputLayouts.h"
#include "../ShaderConstants.h"
#include "Extensions/MatrixExtension.h"
#include "Insanity_Math.h"
#include <assert.h>
#include <algorithm>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;



static void SetMaterial(const InsanityEngine::DX11::Device& device, const Resource<DX11::StaticMesh::Material>& mat);
static void SetMesh(const InsanityEngine::DX11::Device& device, const Resource<DX11::Mesh>& mesh);
static void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::Instance& mesh);

namespace InsanityEngine::DX11::StaticMesh
{
    Renderer::Renderer(DX11::Device& device) :
        m_device(&device)
    {
        m_inputLayouts[positionNormalUVLayout] = DX11::InputLayouts::PositionNormalUV::CreateInputLayout(m_device->GetDevice());
    }


    Component<Instance> Renderer::CreateMesh(ResourceHandle<Mesh> mesh, ResourceHandle<Material> material)
    {

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer<StaticMesh::Constants::VSMesh>(m_device->GetDevice(), &constantBuffer, true);

        Instance data
        {
            .mesh = mesh.GetResourcePointer(),
            .material = material.GetResourcePointer(),
            .constantBuffer = constantBuffer,
            .position{ Math::Types::Scalar{0.f} },
            .scale{ Math::Types::Scalar{1.f} },
            .rotation{}
        };

        m_meshes.push_back(std::make_unique<StaticMesh::Instance>(data));
        return Component<Instance>(*this, *m_meshes.back().get());
    }

    void Renderer::Update()
    {
        for(const auto& mesh : m_meshes)
        {
            //Mesh constants updated
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(mesh->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::VSMesh constants
            { 
                .worldMatrix = Math::Matrix::ScaleRotateTransformMatrix(
                    Math::Matrix::ScaleMatrix(mesh->scale), 
                    mesh->rotation.ToRotationMatrix(), 
                    Math::Matrix::PositionMatrix(mesh->position)) 
            };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(mesh->constantBuffer.Get(), 0);

            //Material constants updated
            m_device->GetDeviceContext()->Map(mesh->material->Get().constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::PSMaterial psConstants{ .color = mesh->material->Get().color };
            std::memcpy(subresource.pData, &psConstants, sizeof(psConstants));
            m_device->GetDeviceContext()->Unmap(mesh->material->Get().constantBuffer.Get(), 0);
        }
    }

    void Renderer::Draw()
    {
        m_device->GetDeviceContext()->IASetInputLayout(m_inputLayouts[positionNormalUVLayout].Get());
        m_device->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //Draw meshes
        for(const auto& mesh : m_meshes)
        {
            SetMaterial(*m_device, *mesh->material);
            SetMesh(*m_device, *mesh->mesh);
            DrawMesh(*m_device, *mesh);
        }

    }

    void Renderer::Destroy(StaticMesh::Instance* object)
    {
        auto it = std::remove_if(m_meshes.begin(), m_meshes.end(), [=](std::unique_ptr<StaticMesh::Instance>& o) { return o.get() == object; });

        if(it != m_meshes.end())
            m_meshes.erase(it, m_meshes.end());
    }

}


void SetMaterial(const InsanityEngine::DX11::Device& device, const Resource<DX11::StaticMesh::Material>& mat)
{


    device.GetDeviceContext()->VSSetShader(mat.Get().shader->GetVertexShader().Get(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.Get().shader->GetPixelShader().Get(), nullptr, 0);

    std::array samplers{ mat.Get().albedo->GetSamplerState().Get() };
    std::array textures{ mat.Get().albedo->GetShaderResource().Get() };
    std::array constantBuffers{ mat.Get().constantBuffer.Get() };
    device.GetDeviceContext()->PSSetSamplers(DX11::StaticMesh::Registers::PS::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources(DX11::StaticMesh::Registers::PS::albedoTexture, 1, textures.data());
    device.GetDeviceContext()->PSSetConstantBuffers(DX11::StaticMesh::Registers::PS::materialConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
}

void SetMesh(const InsanityEngine::DX11::Device& device, const Resource<DX11::Mesh>& mesh)
{
    std::array vertexBuffers{ mesh.Get().vertexBuffer.Get() };

    UINT stride = sizeof(DX11::InputLayouts::PositionNormalUV::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.Get().indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::Instance& mesh)
{
    std::array constantBuffers{ mesh.constantBuffer.Get() };
    device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
    device.GetDeviceContext()->DrawIndexed(mesh.mesh->Get().indexCount, 0, 0);
}