#pragma once

#include "../CommonInclude.h"
#include "Wrappers/ResourceWrapper.h"

namespace InsanityEngine::DX11
{
    struct Shader
    {
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
    };
};

namespace InsanityEngine
{
    template<>
    struct Resource<DX11::Shader>
    {
        DX11::Shader resource;
    };

    template<>
    class ResourceHandle<DX11::Shader> : public SharedResourceHandle<DX11::Shader>
    {
    public:
        using SharedResourceHandle::SharedResourceHandle;
    };
}