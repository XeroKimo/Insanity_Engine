#include "Application.h"
#include "Vector.h"

#include "../DX11/Window.h"
#include "../DX11/Device.h"
#include "../DX11/Helpers.h"
#include "../DX11/ShaderConstants.h"
#include "../DX11/Resources.h"
#include "../Test Scenes/TriangleRender.h"
#include "../Test Scenes/TriangleRenderScene2.h"
#include "../DX11/Renderer/Renderer.h"
#include "Extensions/MatrixExtension.h"
#include "../Factories/ResourceFactory.h"
#include "../DX11/RenderModule.h"
#include "../Factories/ComponentFactory.h"

#include "SDL.h"
#include <chrono>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;


namespace InsanityEngine::Application
{
    Application::Application(DX11::Device& device, DX11::Window& window, DX11::RenderModule& renderer, ResourceFactory& factory, ComponentFactory& componentFactory) :
        m_device(device),
        m_window(window),
        m_renderer(renderer),
        m_factory(factory),
        m_componentFactory(componentFactory)
    {

    }

    int Application::Run()
    {
        
        TriangleRenderSetup2(m_device, m_window, m_factory, m_componentFactory);
        //TriangleRenderSetup(m_device, m_window);

        std::chrono::time_point previous = std::chrono::steady_clock::now();
        std::chrono::time_point now = previous;
        SDL_Event event;

        m_window.clearColor = { 0, 0.3f, 0.7f, 1 };
        
        HRESULT hr;
        DX11::ComPtr<ID3D11Buffer> camData;
        DX11::StaticMesh::Constants::Camera c;
        DX11::Helpers::CreateConstantBuffer<DX11::StaticMesh::Constants::Camera>(m_device.GetDevice(), camData.GetAddressOf(), true); 

        DX11::ComPtr<ID3D11Resource> resource;
        m_window.GetBackBuffer()->GetResource(&resource);

        DX11::ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);

        D3D11_TEXTURE2D_DESC textureDesc = {};
        texture->GetDesc(&textureDesc);

        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        DX11::ComPtr<ID3D11Texture2D> depthStencilTexture;
        hr = m_device.GetDevice()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

        //if(FAILED(hr))
        //{
        //    throw HRESULTException("Failed to create depth stencil texture", hr);
        //}

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = 0;
        depthStencilDesc.Texture2D.MipSlice = 0;

        DX11::ComPtr<ID3D11DepthStencilView> depthStencilView;
        hr = m_device.GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &depthStencilView);

        //if(FAILED(hr))
        //{
        //    throw HRESULTException("Failed to create depth stencil view", hr);
        //}


        D3D11_DEPTH_STENCIL_DESC desc{};

        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = false;
        desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace = desc.FrontFace;

        DX11::ComPtr<ID3D11DepthStencilState> depthStencilState;
        m_device.GetDevice()->CreateDepthStencilState(&desc, &depthStencilState);

        DX11::CameraObject camera{ camData, DX11::CameraData(m_window.GetBackBuffer(), depthStencilView, depthStencilState) };

        camera.data.position.z() = -5;
        D3D11_MAPPED_SUBRESOURCE subresource;
        m_device.GetDeviceContext()->Map(camera.GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

        DX11::StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera.data.GetViewMatrix(), camera.data.GetPerspectiveMatrix()) };
        std::memcpy(subresource.pData, &constants, sizeof(constants));
        m_device.GetDeviceContext()->Unmap(camera.GetConstantBuffer(), 0);


        while(m_running)
        {
            if(SDL_PollEvent(&event))
            {
                TriangleRenderInput2(event);
                //TriangleRenderInput(event);

                if(event.type == SDL_EventType::SDL_QUIT)
                    m_running = false;

               
            }
            else
            {
                previous = std::exchange(now, std::chrono::steady_clock::now());
                float delta = std::chrono::duration<float>(now - previous).count();

                TriangleRenderUpdate2(delta);
                m_window.ClearBackBuffer();

                std::array renderTargets{ static_cast<ID3D11RenderTargetView*>(camera.data.GetRenderTargetView()) };

                Vector2f resolution = camera.data.GetRenderTargetResolution();

                D3D11_VIEWPORT viewport = {};
                viewport.Width = static_cast<float>(resolution.x());
                viewport.Height = static_cast<float>(resolution.y());
                viewport.MaxDepth = 1;
                viewport.MinDepth = 0;

                D3D11_RECT rect = {};
                rect.right = static_cast<LONG>(resolution.x());
                rect.bottom = static_cast<LONG>(resolution.y());

                m_device.GetDeviceContext()->OMSetDepthStencilState(camera.data.GetDepthStencilState(), 0);
                m_device.GetDeviceContext()->ClearDepthStencilView(camera.data.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
                m_device.GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera.data.GetDepthStencilView());

                m_device.GetDeviceContext()->RSSetViewports(1, &viewport);
                m_device.GetDeviceContext()->RSSetScissorRects(1, &rect);

                std::array vsCameraBuffer{ camera.GetConstantBuffer() };
                m_device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::cameraConstants, static_cast<UINT>(vsCameraBuffer.size()), vsCameraBuffer.data());
                m_renderer.Update(delta);
                m_renderer.Draw();
                m_window.Present();
                //TriangleRender(m_device, m_window);
                //m_window.Present();
            }
        }

        TriangleRenderShutdown2();

        return 0;
    }

    void Application::Quit()
    {
        m_running = false;
    }


    int RunApplication()
    {
        int retVal = 0;
        SDL_Init(SDL_INIT_VIDEO);

        try
        {
            ResourceFactory resourceFactory;
            ComponentFactory componentFactory;

            DX11::Device device;
            //DX11::StaticMesh::Renderer renderer{ device };
            DX11::RenderModule renderModule{ resourceFactory, componentFactory, device };
            DX11::Window window{ "Insanity Engine", { 1280.f, 720.f }, device };


            Application app(device, window, renderModule, resourceFactory, componentFactory);
            app.Run();
        }
        catch(std::exception e)
        {
            retVal = 1;
        }

        SDL_Quit();

        return retVal;
    }
}
