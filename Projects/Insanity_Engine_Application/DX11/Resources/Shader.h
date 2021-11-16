#pragma once
#include "../CommonInclude.h"
#include "../Internal/Resources.h"
#include "../../Factories/ResourceFactory.h"

namespace InsanityEngine
{
    template<>
    struct ResourceInitializer<DX11::Shader> : public ResourceInitializer<UnknownResource>
    {
        std::wstring_view vertexShader;
        std::wstring_view pixelShader;
    };


    template<>
    class Resource<DX11::Shader> : public UnknownResource
    {
    private:
        DX11::Shader m_shader;

    public:
        Resource(std::string_view name, DX11::Shader shader);

    public:
        DX11::ComPtr<ID3D11VertexShader> GetVertexShader() const { return m_shader.vertexShader; }
        DX11::ComPtr<ID3D11PixelShader> GetPixelShader() const { return m_shader.pixelShader; }
    };
}