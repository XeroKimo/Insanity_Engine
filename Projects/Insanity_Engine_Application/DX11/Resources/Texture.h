#pragma once
#include "../CommonInclude.h"
#include "../Internal/Resources.h"
#include "../../Factories/ResourceFactory.h"

namespace InsanityEngine
{
    namespace DX11
    {
        class RenderModule;
    }

    template<>
    struct ResourceInitializer<DX11::Texture> : public ResourceInitializer<UnknownResource>
    {
        std::wstring_view textureName;
    };


    template<>
    class Resource<DX11::Texture> : public UnknownResource
    {
    private:
        DX11::Texture m_texture;

    public:
        Resource(std::string_view name, DX11::Texture texture);

    public:
        DX11::ComPtr<ID3D11ShaderResourceView> GetShaderResource() const { return m_texture.shaderResource; }
        DX11::ComPtr<ID3D11SamplerState> GetSamplerState() const { return m_texture.sampler; }
    };



    template<>
    class ResourceHandle<DX11::Texture> : public UserDefinedResourceHandle<DX11::Texture>
    {
        using Base = UserDefinedResourceHandle<DX11::Texture>;
        friend class DX11::RenderModule;

    public:
        using Base::UserDefinedResourceHandle;
    };
}