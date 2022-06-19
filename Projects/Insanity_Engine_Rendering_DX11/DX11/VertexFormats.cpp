#include "VertexFormats.h"
#include <span>

namespace InsanityEngine::Rendering::D3D11::VertexFormat
{
    using Microsoft::WRL::ComPtr;

    namespace Position
    {
        tl::expected<TypedD3D::Wrapper<ID3D11InputLayout>, HRESULT> CreateLayout(gsl::not_null<TypedD3D::Wrapper<ID3D11Device>> device, gsl::not_null<Microsoft::WRL::ComPtr<ID3DBlob>> shaderBlob)
        {
            return device->CreateInputLayout(elements, *shaderBlob.get().Get());
        }
    }

    namespace PositionNormalUV
    {
        tl::expected<TypedD3D::Wrapper<ID3D11InputLayout>, HRESULT> CreateLayout(gsl::not_null<TypedD3D::Wrapper<ID3D11Device>> device, gsl::not_null<Microsoft::WRL::ComPtr<ID3DBlob>> shaderBlob)
        {
            return device->CreateInputLayout(elements, *shaderBlob.get().Get());
        }
    }
}