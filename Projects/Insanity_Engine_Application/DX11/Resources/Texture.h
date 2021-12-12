#pragma once
#include "../CommonInclude.h"
#include "Wrappers/ResourceWrapper.h"
#include <memory>

namespace InsanityEngine::DX11
{
    class Renderer;

    struct Texture
    {
        ComPtr<ID3D11ShaderResourceView> shaderResource;
        //ComPtr<ID3D11SamplerState> samplerState;
    };
}

namespace InsanityEngine
{
    template<>
    struct Resource<DX11::Texture>
    {

        DX11::Texture texture;

        //ID3D11SamplerState& GetSamplerState() const { return *m_texture.samplerState.Get(); }
    };

    template<>
    class ResourceHandle<DX11::Texture> : public SharedResourceHandle<DX11::Texture>
    {
    public:
        using SharedResourceHandle::SharedResourceHandle;
    };
}