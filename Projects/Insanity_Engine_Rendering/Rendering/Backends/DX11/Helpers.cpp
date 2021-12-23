#include "Helpers.h"

namespace  InsanityEngine::Rendering::DX11
{
    using namespace Math::Types;

    Utils::Expected<ComPtr<ID3D11ShaderResourceView>, HRESULT> CreateTexture(ID3D11Device& device, std::wstring_view fileName, DirectX::WIC_FLAGS flags)
    {
        DirectX::TexMetadata metaData;
        DirectX::ScratchImage scratchImage;
        HRESULT hr = DirectX::LoadFromWICFile(fileName.data(), flags, &metaData, scratchImage);

        if(FAILED(hr))
            return Utils::Unexpected(hr);

        const DirectX::Image* image = scratchImage.GetImage(0, 0, 0);
        DirectX::ScratchImage flippedImage;
        hr = DirectX::FlipRotate(*image, DirectX::TEX_FR_FLAGS::TEX_FR_FLIP_VERTICAL, flippedImage);

        if(FAILED(hr))
            return Utils::Unexpected(hr);

        ComPtr<ID3D11ShaderResourceView> texture; 
        DirectX::CreateShaderResourceView(&device, flippedImage.GetImage(0, 0, 0), 1, metaData, texture.GetAddressOf());

        return texture;
    }

    Vector3ui GetTextureSize(ID3D11Resource& resource)
    {
        D3D11_RESOURCE_DIMENSION dimension;
        resource.GetType(&dimension);

        if(dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        {
            ComPtr<ID3D11Texture1D> one;
            resource.QueryInterface(one.GetAddressOf());

            D3D11_TEXTURE1D_DESC oneDesc;

            one->GetDesc(&oneDesc);
            return Vector3ui(oneDesc.Width, 0, 0);
        }
        else if(dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            ComPtr<ID3D11Texture2D> two;
            resource.QueryInterface(two.GetAddressOf());

            D3D11_TEXTURE2D_DESC twoDesc;
            two->GetDesc(&twoDesc);

            return Vector3ui(twoDesc.Width, twoDesc.Height, 0);
        }
        else if(dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
        {
            ComPtr<ID3D11Texture3D> three;
            resource.QueryInterface(three.GetAddressOf());

            D3D11_TEXTURE3D_DESC threeDesc;

            three->GetDesc(&threeDesc);
            return Vector3ui(threeDesc.Width, threeDesc.Height, threeDesc.Depth);
        }

        return Vector3ui();
    }

    Vector3ui GetTextureSize(ID3D11View& view)
    {
        ComPtr<ID3D11Resource> resource;
        view.GetResource(resource.GetAddressOf());

        return GetTextureSize(*resource.Get());
    }
}
