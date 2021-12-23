#pragma once
#include "Common_Include.h"
#include "Insanity_Math.h"

namespace InsanityEngine::Rendering::DX11
{
    class Texture
    {
    private:
        ComPtr<ID3D11ShaderResourceView1> m_resourceView;

    public:
        Texture() = default;
        Texture(ComPtr<ID3D11ShaderResourceView1> resource);

    public:
        
        Math::Types::Vector3ui GetSize() const;

        ID3D11ShaderResourceView1& GetResourceView() const { return *m_resourceView.Get(); }
    };

}