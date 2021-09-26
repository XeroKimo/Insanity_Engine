#pragma once
#include "CommonInclude.h"

namespace InsanityEngine::DX11
{
    class Device
    {
    private:
        ComPtr<ID3D11Device5> m_device;
        ComPtr<ID3D11DeviceContext4> m_deviceContext;
        ComPtr<ID3D11Debug> m_debug;
        ComPtr<ID3D11InfoQueue> m_infoQueue;

    public:
        Device();
        ~Device();


    public:
        ID3D11Device5* GetDevice() const { return m_device.Get(); }
        ID3D11DeviceContext4* GetDeviceContext() const { return m_deviceContext.Get(); }
    };
}