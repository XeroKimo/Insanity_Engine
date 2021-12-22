#include "Helpers.h"

namespace  InsanityEngine::Rendering::DX11
{
    using namespace Math::Types;
    Vector3ui GetTextureDimensions(ID3D11Resource& resource)
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
}
