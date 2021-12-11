#include "RenderModule.h"
#include "Device.h"
#include "Helpers.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "Extensions/MatrixExtension.h"
#include "ShaderConstants.h"

using namespace InsanityEngine::Math::Types;

namespace InsanityEngine::DX11
{
    RenderModule::RenderModule(ResourceFactory& resourceFactory, ComponentFactory& componentFactory, Device& device, Window& window) :
        m_renderer(device),
        m_device(device),
        m_window(window)
    {
        resourceFactory.AddResourceCreationCallback<Texture>([&](const ResourceInitializer<Texture>& init) { return this->CreateTexture(init); });
        resourceFactory.AddResourceCreationCallback<Mesh>([&](const ResourceInitializer<Mesh>& init) { return this->CreateMesh(init); });
        resourceFactory.AddResourceCreationCallback<Shader>([&](const ResourceInitializer<Shader>& init) { return this->CreateShader(init); });
        resourceFactory.AddResourceCreationCallback<StaticMesh::Material>([&](const ResourceInitializer<StaticMesh::Material>& init) { return this->CreateMaterial(init); });

        componentFactory.RegisterComponentCreationCallback<StaticMesh::Instance>([&](const ComponentInitializer<StaticMesh::Instance>& init) { return m_renderer.CreateMesh(init.mesh, init.material); });
        componentFactory.RegisterComponentCreationCallback<Camera>([&](const ComponentInitializer<Camera>& init) { return CreateCamera(init); });


        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = -FLT_MAX;
        samplerDesc.MaxLOD = FLT_MAX;

        HRESULT hr = device.GetDevice()->CreateSamplerState(&samplerDesc, &m_defaultSampler);
        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create sampler state", hr);
        }

        D3D11_DEPTH_STENCIL_DESC desc{};

        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_GREATER;
        desc.StencilEnable = false;
        desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace = desc.FrontFace;

        m_device.GetDevice()->CreateDepthStencilState(&desc, &m_defaultDepthStencilState);

    }
    void RenderModule::Update(float deltaTime)
    {
        for(const auto& camera : m_cameras)
        {
            DX11::StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera->GetViewMatrix(), camera->GetPerspectiveMatrix()) };
           
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device.GetDeviceContext()->Map(camera->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device.GetDeviceContext()->Unmap(camera->constantBuffer.Get(), 0);
        }

        m_renderer.Update();
    }
    void RenderModule::Draw()
    {
        auto render = [&](Camera& camera)
        {

            std::array renderTargets = std::to_array<ID3D11RenderTargetView*>(
                {
                    camera.renderTargetView.Get()
                });
            Vector2f targetResolution = Helpers::GetTextureResolution(*renderTargets[0]);

            D3D11_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(targetResolution.x());
            viewport.Height = static_cast<float>(targetResolution.y());
            viewport.MaxDepth = 1;
            viewport.MinDepth = 0;

            D3D11_RECT rect = {};
            rect.right = static_cast<LONG>(targetResolution.x());
            rect.bottom = static_cast<LONG>(targetResolution.y());

            Helpers::ClearRenderTargetView(m_device.GetDeviceContext(), camera.renderTargetView.Get(), { 0, 0.3f, 0.7f, 1 });
            m_device.GetDeviceContext()->ClearDepthStencilView(camera.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0, 0);

            m_device.GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera.depthStencilView.Get());
            m_device.GetDeviceContext()->OMSetDepthStencilState(camera.depthStencilState.Get(), 0);
            m_device.GetDeviceContext()->RSSetViewports(1, &viewport);
            m_device.GetDeviceContext()->RSSetScissorRects(1, &rect);

            std::array vsCameraBuffer{ camera.constantBuffer.Get() };
            m_device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::cameraConstants, static_cast<UINT>(vsCameraBuffer.size()), vsCameraBuffer.data());

            m_renderer.Draw();
        };

        if(m_mainCamera)
        {
            render(*m_mainCamera);
        }

        for(const auto& camera : m_cameras)
        {
            if(camera.get() == m_mainCamera)
                continue;

            if(camera->renderTargetView == nullptr)
                continue;

            render(*camera);

        }
    }

    std::shared_ptr<Resource<Texture>> RenderModule::CreateTexture(const ResourceInitializer<Texture>& initializer)
    {
        return std::make_shared<Resource<Texture>>(initializer.name, DX11::CreateTexture(m_device.GetDevice(), initializer.textureName, m_defaultSampler));
    }

    std::shared_ptr<Resource<Mesh>> RenderModule::CreateMesh(const ResourceInitializer<Mesh>& initializer)
    {
        using init_type = ResourceInitializer<Mesh>;
        if(std::holds_alternative<init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>>(initializer.data))
        {
            const init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>& mesh = std::get<init_type::RawInit<InputLayouts::PositionNormalUV::VertexData>>(initializer.data);

            ComPtr<ID3D11Buffer> vertexBuffer;
            HRESULT hr = Helpers::CreateVertexBuffer(m_device.GetDevice(), &vertexBuffer, mesh.vertices, Helpers::VertexBufferUsage::Immutable);

            if(FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create Vertex Buffer", hr);
            }

            ComPtr<ID3D11Buffer> indexBuffer;
            hr = Helpers::CreateIndexBuffer(m_device.GetDevice(), &indexBuffer, mesh.indices);

            if(FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create Index Buffer", hr);
            }

            Mesh m(vertexBuffer, indexBuffer, static_cast<UINT>(mesh.vertices.size()), static_cast<UINT>(mesh.indices.size()));
            return std::make_shared<Resource<Mesh>>(initializer.name, m);

        }

        return nullptr;
    }

    std::shared_ptr<Resource<Shader>> RenderModule::CreateShader(const ResourceInitializer<Shader>& initializer)
    {

        return std::make_shared<Resource<Shader>>(initializer.name, DX11::CreateShader(m_device.GetDevice(), initializer.vertexShader, initializer.pixelShader));
    }
    std::shared_ptr<Resource<StaticMesh::Material>> RenderModule::CreateMaterial(const ResourceInitializer<StaticMesh::Material>& initializer)
    {
        ComPtr<ID3D11Buffer> constantBuffer;
        DX11::StaticMesh::Constants::PSMaterial constants{ .color = initializer.color };
        Helpers::CreateConstantBuffer(m_device.GetDevice(), &constantBuffer, true, constants);

        StaticMesh::Material m(constantBuffer, initializer.shader.GetResourcePointer(), initializer.albedo.GetResourcePointer(), initializer.color);
        return std::make_shared<Resource<StaticMesh::Material>>(initializer.name, m);
    }

    Component<Camera> RenderModule::CreateCamera(const ComponentInitializer<Camera>& initializer)
    {
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();

        Math::Types::Vector2f depthBufferSize;


        if(initializer.optionalTexture != nullptr)
        {
            //DX11::ComPtr<ID3D11Resource> resource;

            //initializer.optionalTexture.GetResource().GetShaderResource()->GetResource(&resource);

            //DX11::ComPtr<ID3D11Texture2D> texture;
            //resource.As(&texture);

            //D3D11_TEXTURE2D_DESC textureDesc = {};
            //texture->GetDesc(&textureDesc);

            //depthBufferSize.x() = static_cast<float>(textureDesc.Width);
            //depthBufferSize.y() = static_cast<float>(textureDesc.Height);
        }
        else
        {
            if(m_mainCamera == nullptr)
            {
                m_mainCamera = camera.get();
            }

            m_mainCamera->renderTargetView = ComPtr<ID3D11RenderTargetView>(m_window.GetBackBuffer());
        }

        depthBufferSize = Helpers::GetTextureResolution(*camera->renderTargetView.Get());
        DX11::StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera->GetViewMatrix(), camera->GetPerspectiveMatrix()) };
        Helpers::CreateConstantBuffer(m_device.GetDevice(), camera->constantBuffer.GetAddressOf(), true, constants);

        D3D11_TEXTURE2D_DESC textureDesc = {};

        textureDesc.Width = static_cast<UINT>(depthBufferSize.x());
        textureDesc.Height = static_cast<UINT>(depthBufferSize.y());
        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        DX11::ComPtr<ID3D11Texture2D> depthStencilTexture;
        HRESULT hr = m_device.GetDevice()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = 0;
        depthStencilDesc.Texture2D.MipSlice = 0;

        DX11::ComPtr<ID3D11DepthStencilView> depthStencilView;
        hr = m_device.GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &depthStencilView);


        camera->depthStencilState = m_defaultDepthStencilState;
        camera->depthStencilView = depthStencilView;

        m_cameras.push_back(std::move(camera));

        return Component<Camera>(*this, *m_cameras.back());
    }

    void RenderModule::Destroy(Camera* camera)
    {
        auto it = std::remove_if(m_cameras.begin(), m_cameras.end(), [&](const auto& comp) { return comp.get() == camera; });
        std::unique_ptr<Camera> deletedCamera = std::move(*it);

        m_cameras.erase(it, m_cameras.end());

        if(camera == m_mainCamera)
        {
            auto it = std::find_if(m_cameras.begin(), m_cameras.end(), [&](const std::unique_ptr<Camera>& comp) { return comp->renderTargetView.Get() == m_window.GetBackBuffer(); });

            if(it == m_cameras.end())
                return;

            m_mainCamera = it->get();

        }
    }
}