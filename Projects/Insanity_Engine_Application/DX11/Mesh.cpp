#include "Mesh.h"
#include "Extensions/MatrixExtension.h"
#include "Device.h"
#include <assert.h>


namespace InsanityEngine::DX11::StaticMesh
{
    using namespace Math::Types;

    std::array<D3D11_INPUT_ELEMENT_DESC, 3> GetInputElementDescription()
    {
        std::array<D3D11_INPUT_ELEMENT_DESC, 3> inputLayout;

        inputLayout[0].SemanticName = "POSITION";
        inputLayout[0].SemanticIndex = 0;
        inputLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputLayout[0].InputSlot = 0;
        inputLayout[0].AlignedByteOffset = 0;
        inputLayout[0].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
        inputLayout[0].InstanceDataStepRate = 0;

        inputLayout[1].SemanticName = "NORMAL";
        inputLayout[1].SemanticIndex = 0;
        inputLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputLayout[1].InputSlot = 0;
        inputLayout[1].AlignedByteOffset = sizeof(Math::Types::Vector3f);
        inputLayout[1].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
        inputLayout[1].InstanceDataStepRate = 0;

        inputLayout[2].SemanticName = "TEXCOORD";
        inputLayout[2].SemanticIndex = 0;
        inputLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
        inputLayout[2].InputSlot = 0;
        inputLayout[2].AlignedByteOffset = sizeof(Math::Types::Vector3f) * 2;
        inputLayout[2].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
        inputLayout[2].InstanceDataStepRate = 0;

        return inputLayout;
    }


    Mesh::Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount) :
        m_vertexBuffer(std::move(vertexBuffer)),
        m_vertexCount(std::move(vertexCount)),
        m_indexBuffer(std::move(indexBuffer)),
        m_indexCount(std::move(indexCount))
    {
        assert(m_vertexBuffer != nullptr);
        assert(m_indexBuffer != nullptr);
    }


    Texture::Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState) :
        m_shaderResourceView(std::move(shaderResourceView)),
        m_samplerState(std::move(samplerState))
    {
        assert(m_shaderResourceView != nullptr);
        assert(m_samplerState != nullptr);
    }

    void Texture::SetSamplerState(ComPtr<ID3D11SamplerState> samplerState)
    {
        assert(samplerState != nullptr);

        m_samplerState = std::move(samplerState);
    }

    Shader::Shader(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader) :
        m_vertexShader(std::move(vertexShader)),
        m_pixelShader(std::move(pixelShader))
    {
        assert(m_vertexShader != nullptr);
        assert(m_pixelShader != nullptr);
    }

    Material::Material(std::shared_ptr<Shader> shader, std::shared_ptr<Texture> albedo, Vector4f color) :
        m_shader(std::move(shader)),
        m_albedo(std::move(albedo)),
        color(color)
    {

    }

    void Material::SetShader(std::shared_ptr<Shader> shader)
    {
        assert(shader != nullptr);

        m_shader = std::move(shader);
    }

    void Material::SetAlbedo(std::shared_ptr<Texture> albedo)
    {
        assert(albedo != nullptr);

        m_albedo = std::move(albedo);
    }

    MeshObject::MeshObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
        m_mesh(std::move(mesh)),
        m_material(std::move(material))
    {

    }

    void MeshObject::SetMesh(std::shared_ptr<Mesh> mesh)
    {
        assert(mesh != nullptr);
        m_mesh = std::move(mesh);
    }

    void MeshObject::SetMaterial(std::shared_ptr<Material> material)
    {
        assert(material != nullptr);
        m_material = std::move(material);
    }

    Matrix4x4f MeshObject::GetObjectMatrix() const
    {
        return Math::Matrix::PositionMatrix(position) * quat.ToRotationMatrix();
    }

}