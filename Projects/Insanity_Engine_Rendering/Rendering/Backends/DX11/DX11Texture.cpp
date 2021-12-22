#include "DX11Texture.h"
#include "Helpers.h"


namespace InsanityEngine::Rendering::DX11
{
    using namespace Math::Types;

    Texture::Texture(ComPtr<ID3D11ShaderResourceView1> resource) :
        m_resourceView(resource)
    {
    }

    Vector3ui Texture::GetTextureDimensions() const
    {
        if(m_resourceView == nullptr)
            return {};

        ComPtr<ID3D11Resource> resource;
        m_resourceView->GetResource(&resource);

        return Helpers::Rendering::DX11::GetTextureDimensions(*resource.Get());
    }

}