#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"
#include "DirectXTex/DirectXTex.h"
#include <span>
#include <string_view>
#include <optional>

namespace InsanityEngine::DX11::Helpers
{
    extern void ClearRenderTargetView(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* renderTargetView, Math::Types::Vector4f color);

    enum class VertexBufferUsage
    {
        Default = D3D11_USAGE_DEFAULT,
        Immutable = D3D11_USAGE_IMMUTABLE,
        Dynamic = D3D11_USAGE_DYNAMIC
    };

    enum class IndexBufferUsage
    {
        Default = D3D11_USAGE_DEFAULT,
        Immutable = D3D11_USAGE_IMMUTABLE,
        Dynamic = D3D11_USAGE_DYNAMIC
    };

    extern HRESULT CreateVertexBuffer(ID3D11Device* device, ID3D11Buffer** buffer, void* vertexData, size_t vertexCount, size_t sizeOfVertexType,  VertexBufferUsage usage = VertexBufferUsage::Default, bool streamOut = false);


    template<class VertexType, size_t Extents = std::dynamic_extent>
    HRESULT CreateVertexBuffer(ID3D11Device* device, ID3D11Buffer** buffer, std::span<VertexType, Extents> vertexSpan,  VertexBufferUsage usage = VertexBufferUsage::Default, bool streamOut = false)
    {
        return CreateVertexBuffer(device, buffer, vertexSpan.data(), vertexSpan.size(), sizeof(VertexType), usage, streamOut);
    }


    template<size_t Extents = std::dynamic_extent>
    HRESULT CreateIndexBuffer(ID3D11Device* device, ID3D11Buffer** buffer, std::span<UINT, Extents> indexSpan,  IndexBufferUsage usage = IndexBufferUsage::Default)
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = static_cast<UINT>(indexSpan.size_bytes());
        bufferDesc.Usage = static_cast<D3D11_USAGE>(usage);
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = (usage == IndexBufferUsage::Dynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = indexSpan.data();
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;

        return device->CreateBuffer(&bufferDesc, &data, buffer);
    }

    extern HRESULT CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, UINT bufferSize, bool isDynamic, const void* startingData);

    template<class T>
    HRESULT CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, bool isDynamic, const T& startingData)
    {
        return CreateConstantBuffer(device, buffer, sizeof(T), isDynamic, &startingData);
    }

    template<class T>
    HRESULT CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, bool isDynamic)
    {
        return CreateConstantBuffer(device, buffer, sizeof(T), isDynamic, nullptr);
    }

    extern HRESULT CreateTextureFromFile(ID3D11Device* device, ID3D11ShaderResourceView** shaderResourceView, std::wstring_view file, DirectX::WIC_FLAGS flags);

    extern Math::Types::Vector2f GetTextureResolution(ID3D11RenderTargetView& renderTarget);
    extern Math::Types::Vector2f GetTextureResolution(ID3D11Texture2D& texture);
}