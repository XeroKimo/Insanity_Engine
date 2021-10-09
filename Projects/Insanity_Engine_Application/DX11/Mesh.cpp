#include "Mesh.h"
#include "Extensions/MatrixExtension.h"
#include <assert.h>

namespace InsanityEngine::DX11::StaticMesh
{
    Mesh::Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount) :
        m_vertexBuffer(vertexBuffer),
        m_vertexCount(vertexCount),
        m_indexBuffer(indexBuffer),
        m_indexCount(indexCount)
    {
    }

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

    Texture::Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState) :
        m_shaderResourceView(shaderResourceView),
        m_samplerState(samplerState)
    {
    }

    void Texture::SetSamplerState(ComPtr<ID3D11SamplerState> samplerState)
    {
        assert(samplerState != nullptr);

        m_samplerState = samplerState;
    }

    Material::Material(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader, Texture albedo, Vector4f color) :
        m_vertexShader(vertexShader),
        m_pixelShader(pixelShader),
        albedo(albedo),
        color(color)
    {

    }

    MeshObject::MeshObject(Mesh mesh, Material material) :
        mesh(mesh),
        material(material)
    {

    }

    Matrix4x4f MeshObject::GetObjectMatrix() const
    {
        return Math::Matrix::PositionMatrix(position) * quat.ToRotationMatrix();
    }
}