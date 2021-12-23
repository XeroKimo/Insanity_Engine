#pragma once

#include "Common_Include.h"
#include "Insanity_Math.h"
#include "Utils/Expected.h"
#include "DirectXTex/DirectXTex.h"
#include <string_view>
#include <functional>

namespace InsanityEngine::Helpers::Rendering::DX11
{
    using namespace InsanityEngine::Rendering::DX11;



//Texture related helpers
    Utils::Expected<ComPtr<ID3D11ShaderResourceView>, HRESULT> CreateTexture(ID3D11Device& device, std::wstring_view fileName, DirectX::WIC_FLAGS flags);

    Math::Types::Vector3ui GetTextureSize(ID3D11Resource& resource);
    Math::Types::Vector3ui GetTextureSize(ID3D11View& view);
}