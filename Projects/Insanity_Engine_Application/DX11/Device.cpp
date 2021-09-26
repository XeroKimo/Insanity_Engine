#include "Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"

using namespace InsanityEngine::Debug::Exceptions;

namespace InsanityEngine::DX11
{
    Device::Device()
    {
        ComPtr<ID3D11Device> tempDevice;
        ComPtr<ID3D11DeviceContext> tempDeviceContext;


#ifdef INSANITY_DEBUG
        UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
        UINT flags = 0;
#endif


        D3D_FEATURE_LEVEL levels = D3D_FEATURE_LEVEL_11_1;
        HRESULT hr = D3D11CreateDevice(
            nullptr, 
            D3D_DRIVER_TYPE_HARDWARE, 
            nullptr, 
            flags, 
            &levels, 
            1, 
            D3D11_SDK_VERSION, 
            tempDevice.GetAddressOf(), 
            nullptr, 
            tempDeviceContext.GetAddressOf());

        if(FAILED(hr))
        {
            throw HRESULTException("Failed to create device", hr);
        }

        hr = tempDevice.As(&m_device);
        if(FAILED(hr))
        {
            throw HRESULTException("Failed to query device", hr);
        }

        hr = tempDeviceContext.As(&m_deviceContext);
        if(FAILED(hr))
        {
            throw HRESULTException("Failed to query device context", hr);
        }


#ifdef INSANITY_DEBUG
        hr = tempDevice.As(&m_debug);
        if(FAILED(hr))
        {
            throw HRESULTException("Failed to query debug", hr);
        }

        hr = tempDevice.As(&m_infoQueue);
        if(FAILED(hr))
        {
            throw HRESULTException("Failed to query info queue", hr);
        }

        m_infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        m_infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif

    }

    Device::~Device()
    {
        m_deviceContext->ClearState();
        m_deviceContext->Flush();
#ifdef INSANITY_DEBUG
        m_debug->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL | D3D11_RLDO_FLAGS::D3D11_RLDO_SUMMARY | D3D11_RLDO_FLAGS::D3D11_RLDO_IGNORE_INTERNAL);
#endif
    }
}