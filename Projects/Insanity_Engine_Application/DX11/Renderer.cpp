#include "Renderer.h"
#include "Device.h"
#include "Insanity_Math.h"
#include "Extensions/MatrixExtension.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "Helpers.h"
#include "ShaderConstants.h"

namespace InsanityEngine::DX11
{
    using namespace Math::Types;

    Renderer::Renderer(Device& device, ComPtr<IDXGISwapChain> swapChain) :
        m_device(&device)
    {
        swapChain.As(&m_swapChain);
        InitializeBackBuffer();

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

        HRESULT hr = m_device->GetDevice().CreateSamplerState(&samplerDesc, &m_samplerState);
        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create sampler state", hr);
        }

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

        m_device->GetDevice().CreateDepthStencilState(&desc, &m_depthState);

        
        m_staticMeshInputLayout = InputLayouts::PositionNormalUV::CreateInputLayout(&GetDevice());
    }

    Renderer::~Renderer()
    {
        if(m_drawCallbacks != nullptr)
            m_drawCallbacks->OnUnbinded(*this);

        m_swapChain->SetFullscreenState(false, nullptr);
    }

    void Renderer::Update()
    {
        for(const auto& mesh : m_meshInstances)
        {
            //Mesh constants updated
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext().Map(mesh->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            ShaderConstants::VSMesh constants
            {
                .worldMatrix = Math::Matrix::ScaleRotateTransformMatrix(
                    Math::Matrix::ScaleMatrix(mesh->data.scale),
                    mesh->data.rotation.ToRotationMatrix(),
                    Math::Matrix::PositionMatrix(mesh->data.position))
            };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext().Unmap(mesh->constantBuffer.Get(), 0);

            //Material constants updated
            m_device->GetDeviceContext().Map(mesh->data.material.GetUnderlyingResource()->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            ShaderConstants::PSMaterial psConstants
            { 
                .color = mesh->data.material.GetColor() 
            };
            std::memcpy(subresource.pData, &psConstants, sizeof(psConstants));
            m_device->GetDeviceContext().Unmap(mesh->data.material.GetUnderlyingResource()->constantBuffer.Get(), 0);
        }

        m_drawCallbacks->OnUpdate(*this);
    }

    void Renderer::Draw()
    {
        auto& deviceContext = m_device->GetDeviceContext();
        Math::Types::Vector4f clearColor { 0, 0.3f, 0.7f, 1 };
        deviceContext.ClearRenderTargetView(m_backBuffer.Get(), clearColor.data.data());

        m_drawCallbacks->OnDraw(*this);


        GetSwapChain().Present(1, 0);
    }

    ID3D11Device5& Renderer::GetDevice() const
    {
        return m_device->GetDevice();
    }

    ID3D11DeviceContext4& Renderer::GetDeviceContext() const
    {
        return m_device->GetDeviceContext();
    }

    void Renderer::UpdateCameraData(Component<Camera>& camera)
    {
        ShaderConstants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera.GetViewMatrix(), camera.GetPerspectiveMatrix()) };

        D3D11_MAPPED_SUBRESOURCE subresource;
        m_device->GetDeviceContext().Map(camera.constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
        std::memcpy(subresource.pData, &constants, sizeof(constants));
        m_device->GetDeviceContext().Unmap(camera.constantBuffer.Get(), 0);
    }

    void Renderer::ClearCameraBuffer(Component<Camera>& camera, Math::Types::Vector4f color)
    {
        auto& deviceContext = m_device->GetDeviceContext();
        deviceContext.ClearRenderTargetView(camera.renderTargetView.Get(), color.data.data());
        deviceContext.ClearDepthStencilView(camera.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    }

    void Renderer::RenderMeshes(Component<Camera>& camera)
    {
        auto& deviceContext = m_device->GetDeviceContext();

        auto renderTargets = std::to_array<ID3D11RenderTargetView*>(
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
        std::array vsCameraBuffer{ camera.constantBuffer.Get() };

        deviceContext.OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera.depthStencilView.Get());
        deviceContext.OMSetDepthStencilState(m_depthState.Get(), 0);
        deviceContext.RSSetViewports(1, &viewport);
        deviceContext.RSSetScissorRects(1, &rect);
        deviceContext.VSSetConstantBuffers(ShaderConstants::Registers::VS::cameraConstants, static_cast<UINT>(vsCameraBuffer.size()), vsCameraBuffer.data());

        deviceContext.IASetInputLayout(m_staticMeshInputLayout.Get());
        deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for(const auto& mesh : m_meshInstances)
        {
            std::array samplers{ m_samplerState.Get() };
            std::array textures{ mesh->data.material.GetUnderlyingResource()->resource.texture.GetUnderlyingResource()->texture.shaderResource.Get() };
            std::array materialConstantBuffers{ mesh->constantBuffer.Get() };

            deviceContext.VSSetShader(mesh->data.material.Get().resource.shader.Get().resource.vertexShader.Get(), nullptr, 0);
            deviceContext.PSSetShader(mesh->data.material.Get().resource.shader.Get().resource.pixelShader.Get(), nullptr, 0);

            deviceContext.PSSetSamplers(ShaderConstants::Registers::PS::albedoSampler, 1, samplers.data());
            deviceContext.PSSetShaderResources(ShaderConstants::Registers::PS::albedoTexture, 1, textures.data());
            deviceContext.PSSetConstantBuffers(ShaderConstants::Registers::PS::materialConstants, static_cast<UINT>(materialConstantBuffers.size()), materialConstantBuffers.data());


            std::array vertexBuffers{ mesh->data.mesh.GetUnderlyingResource()->resource.vertexBuffer.Get() };

            UINT stride = sizeof(DX11::InputLayouts::PositionNormalUV::VertexData);
            UINT offset = 0;

            deviceContext.IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
            deviceContext.IASetIndexBuffer(mesh->data.mesh.GetUnderlyingResource()->resource.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

            std::array constantBuffers{ mesh->constantBuffer.Get() };
            deviceContext.VSSetConstantBuffers(ShaderConstants::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
            deviceContext.DrawIndexed(mesh->data.mesh.GetIndexCount(), 0, 0);

        }
    }

    ResourceHandle<Mesh> Renderer::CreateStaticMesh(std::span<InputLayouts::PositionNormalUV::VertexData> vertices, std::span<UINT> indices)
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        Helpers::CreateVertexBuffer(&m_device->GetDevice(), &vertexBuffer, vertices);

        ComPtr<ID3D11Buffer> indexBuffer;
        Helpers::CreateIndexBuffer(&m_device->GetDevice(), &indexBuffer, indices);

        std::shared_ptr<Resource<Mesh>> mesh = std::make_shared<Resource<Mesh>>();
        mesh->resource.vertexBuffer = vertexBuffer;
        mesh->resource.indexBuffer = indexBuffer;
        mesh->resource.vertexCount = static_cast<UINT>(vertices.size());
        mesh->resource.indexCount = static_cast<UINT>(indices.size());

        return ResourceHandle<Mesh>(mesh);
    }

    ResourceHandle<StaticMesh::Material> Renderer::CreateMaterial(ResourceHandle<Texture> texture, ResourceHandle<Shader> shader)
    {
        std::shared_ptr<Resource<StaticMesh::Material>> material = std::make_shared<Resource<StaticMesh::Material>>();
        material->resource.texture = texture;
        material->resource.shader = shader;

        Helpers::CreateConstantBuffer<DX11::ShaderConstants::PSMaterial>(&m_device->GetDevice(), &material->constantBuffer, true);

        return ResourceHandle<StaticMesh::Material>(material);
    }

    ResourceHandle<Texture> Renderer::CreateTexture(std::wstring_view fileName)
    {
        ComPtr<ID3D11ShaderResourceView> resourceView;
        Helpers::CreateTextureFromFile(&m_device->GetDevice(), &resourceView, fileName, DirectX::WIC_FLAGS::WIC_FLAGS_NONE); 

        std::shared_ptr<Resource<Texture>> texture = std::make_shared<Resource<Texture>>();
        texture->texture.shaderResource = resourceView;

        return ResourceHandle<Texture>(texture);
    }

    ResourceHandle<Shader> Renderer::CreateShader(std::wstring_view vertexShader, std::wstring_view pixelShader)
    {
        ComPtr<ID3DBlob> data;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3DCompileFromFile(
            vertexShader.data(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "vs_5_0",
            0,
            0,
            &data,
            &error);


        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to compile vertex shader: " + std::string(errorMsg), hr);
        }


        ComPtr<ID3D11VertexShader> vs;
        hr = m_device->GetDevice().CreateVertexShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &vs);

        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to create vertex shader: " + std::string(errorMsg), hr);
        }

        hr = D3DCompileFromFile(
            pixelShader.data(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ps_5_0",
            0,
            0,
            &data,
            &error);

        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to compile pixel shader: " + std::string(errorMsg), hr);
        }


        ComPtr<ID3D11PixelShader> ps;
        hr = m_device->GetDevice().CreatePixelShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, &ps);
        if(FAILED(hr))
        {
            std::string_view errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
            throw Debug::Exceptions::HRESULTException("Failed to create pixel shader:" + std::string(errorMsg), hr);
        }

        std::shared_ptr<Resource<Shader>> shader = std::make_shared<Resource<Shader>>();
        shader->resource.vertexShader = vs;
        shader->resource.pixelShader = ps;

        return ResourceHandle<Shader>(shader);
    }

    ComponentHandle<StaticMesh::Instance> Renderer::Create(ResourceHandle<Mesh> mesh, ResourceHandle<StaticMesh::Material> material)
    {
        m_meshInstances.push_back(std::make_unique<Component<StaticMesh::Instance>>());

        m_meshInstances.back()->data.mesh = mesh;
        m_meshInstances.back()->data.material = material;

        Helpers::CreateConstantBuffer<ShaderConstants::VSMesh>(&m_device->GetDevice(), &m_meshInstances.back()->constantBuffer, true);
        return ComponentHandle<StaticMesh::Instance>(m_meshInstances.back().get(), this);
    }

    Component<DX11::Camera> Renderer::CreateCamera()
    {
        Component<DX11::Camera> camera;

        camera.renderTargetView = m_backBuffer;
        Helpers::CreateConstantBuffer<ShaderConstants::Camera>(&m_device->GetDevice(), &camera.constantBuffer, true);


        Vector2f depthBufferSize = Helpers::GetTextureResolution(*m_backBuffer.Get());
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
        HRESULT hr = m_device->GetDevice().CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = 0;
        depthStencilDesc.Texture2D.MipSlice = 0;

        hr = m_device->GetDevice().CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &camera.depthStencilView);

        return camera;
    }

    void Renderer::InitializeBackBuffer()
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if(FAILED(hr))
            throw Debug::Exceptions::HRESULTException("Failed to get back buffer. HRESULT: ", hr);

        D3D11_RENDER_TARGET_VIEW_DESC1 rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        hr = m_device->GetDevice().CreateRenderTargetView1(backBuffer.Get(), &rtvDesc, &m_backBuffer);

        if(FAILED(hr))
            throw Debug::Exceptions::HRESULTException("Failed to create back buffer. HRESULT: ", hr);
    }
}
