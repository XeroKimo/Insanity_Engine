#pragma once
#include "CommonInclude.h"
#include "Device.h"
#include "Insanity_Math.h"
#include "Window.h"
#include <string_view>
#include <vector>

namespace InsanityEngine::DX11
{
    class RenderModule
    {
    private:
        Device m_device;

    public:
        template<class RendererTy>
        Window<RendererTy> WindowCreate(std::string_view windowName, Math::Types::Vector2f windowSize)
        {
            return Window<RendererTy>(m_device, windowName, windowSize);
        }

    public:
        ID3D11Device5& GetDevice() const { return m_device.GetDevice(); }
        ID3D11DeviceContext4& GetDeviceContext() const { return m_device.GetDeviceContext(); }
    };
}