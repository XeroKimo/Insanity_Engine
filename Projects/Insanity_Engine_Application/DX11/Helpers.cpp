#include "Helpers.h"

using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::DX11::Helpers
{
    void ClearRenderTargetView(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* renderTargetView, Vector4f color)
    {
        deviceContext->ClearRenderTargetView(renderTargetView, color.data.data());
    }

    HRESULT CreateVertexBuffer(ID3D11Device* device, ID3D11Buffer** buffer, void* vertexData, size_t vertexCount, size_t sizeOfVertexType, VertexBufferUsage usage, bool streamOut)
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = static_cast<UINT>(vertexCount * sizeOfVertexType);
        bufferDesc.Usage = static_cast<D3D11_USAGE>(usage);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        bufferDesc.BindFlags |= (streamOut) ? D3D11_BIND_STREAM_OUTPUT : 0;

        bufferDesc.CPUAccessFlags = (usage == VertexBufferUsage::Dynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = vertexData;
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;

        return device->CreateBuffer(&bufferDesc, &data, buffer);
    }

    HRESULT CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, UINT bufferSize, const void* startingData, bool isDynamic)
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = (bufferSize + 15) & ~15;
        bufferDesc.Usage = (isDynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = (isDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = startingData;
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;

        return device->CreateBuffer(&bufferDesc, &data, buffer);
    }

    HRESULT CreateTextureFromFile(ID3D11Device* device, ID3D11ShaderResourceView** shaderResourceView, std::wstring_view file, DirectX::WIC_FLAGS flags)
    {
        DirectX::TexMetadata metaData;
        DirectX::ScratchImage scratchImage;
        HRESULT hr = DirectX::LoadFromWICFile(file.data(), flags, &metaData, scratchImage);
       
        if(FAILED(hr))
            return hr;

        const DirectX::Image* image = scratchImage.GetImage(0, 0, 0);
        DirectX::ScratchImage flippedImage;
        hr = DirectX::FlipRotate(*image, DirectX::TEX_FR_FLAGS::TEX_FR_FLIP_VERTICAL, flippedImage);

        if(FAILED(hr))
            return hr;

        return DirectX::CreateShaderResourceView(device, flippedImage.GetImage(0, 0, 0), 1, metaData, shaderResourceView);
    }
}