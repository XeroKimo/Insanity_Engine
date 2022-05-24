#pragma once
#include "TypedD3D12.h"
#include <d3d11_4.h>
#include <memory>

namespace InsanityEngine::Rendering
{
    enum class RenderAPI
    {
        DX11 = 0,
        DX12 = 1,
    };

    class Device
    {
        class Concept
        {
        public:
            virtual ~Concept() = default;
        };

    private:
        class DX11 : public Concept
        {
            friend class Device;
        private:
            Microsoft::WRL::ComPtr<ID3D11Device5> m_device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_deviceContext;
        public:
            DX11();

        public:
            ID3D11Device5& Device() const { return *m_device.Get(); }
            ID3D11DeviceContext4& DeviceContext() const { return *m_deviceContext.Get(); }
        };

        class DX12 : public Concept
        {
        private:
            TypedD3D::D3D12::Device5 m_device;

        public:
            DX12();

        public:
            TypedD3D::D3D12::Device5& Device() { return m_device; }
        };

    private:
        template<RenderAPI api>
        struct APIMap;

        template<>
        struct APIMap<RenderAPI::DX11>
        {
            using type = DX11;
        };

        template<>
        struct APIMap<RenderAPI::DX12>
        {
            using type = DX12;
        };


    private:
        RenderAPI m_api;
        std::unique_ptr<Concept> m_device;

    public:
        Device(RenderAPI api) :
            m_api(api),
            m_device(CreateDevice(api))
        {
        }

    public:
        RenderAPI GetType() const { return m_api; }

        template<RenderAPI API>
        typename APIMap<API>::type* As()
        {
            if(API != m_api)
                return nullptr;

            return static_cast<typename APIMap<API>::type*>(m_device.get());
        }

    private:
        std::unique_ptr<Concept> CreateDevice(RenderAPI api)
        {
            switch(api)
            {
            case RenderAPI::DX11:
                return std::make_unique<DX11>();
                break;
            case RenderAPI::DX12:
                return std::make_unique<DX12>();
                break;
            }

            return nullptr;
        }
    };
}