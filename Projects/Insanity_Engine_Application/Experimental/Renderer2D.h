#pragma once
#include "../Rendering/d3d12/Backend.h"
#include "../Rendering/d3d12/BlendConstants.h"
#include "../Rendering/d3d12/VertexFormats.h"
#include "../Rendering/d3d12/ConstantBuffer.h"
#include <TypedD3D.h>
#include <gsl/gsl>
#include <d3dcompiler.h>

namespace InsanityEngine::Experimental::Rendering
{
    using namespace InsanityEngine::Rendering;

    struct Texture
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    };

    struct Sprite
    {
        Math::Types::Vector3f position;
        Math::Types::Vector3f scale = Math::Types::Scalar(1);
        Texture texture;
    };

    class SpriteRenderer
    {
        template<class Ty>
        friend struct Deleter;

        template<class Ty>
        friend struct Handle;

        using Vertex = D3D12::VertexFormat::PositionUV::Format;

    private:
        template<class Ty>
        struct Deleter
        {
            SpriteRenderer* renderer;

            void operator()(Ty* obj) const { renderer->Destroy(obj); }
        };

        template<class Ty>
        struct Managed;

    public:
        template<class Ty>
        class Handle;

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_spriteRootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_spritePipeline;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_spriteMesh;
        TypedD3D::D3D12::DescriptorHeap::Sampler m_spriteSampler;
        TypedD3D::D3D12::DescriptorHeap::CBV_SRV_UAV m_textures;

        std::vector<std::unique_ptr<Managed<Sprite>>> m_sprites;
        Texture m_defaultTexture;

    public:
        SpriteRenderer(D3D12::Backend& backend)
        {
            D3D12_DESCRIPTOR_RANGE range
            {
                .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
                .NumDescriptors = 1,
                .BaseShaderRegister = 0,
                .RegisterSpace = 0,
                .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
            };
            D3D12_DESCRIPTOR_RANGE range2
            {
                .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                .NumDescriptors = 1,
                .BaseShaderRegister = 0,
                .RegisterSpace = 0,
                .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
            };
            auto rootSignatureParams = std::to_array<D3D12_ROOT_PARAMETER>(
                {
                    D3D12_ROOT_PARAMETER
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &range2
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
                    },
                    D3D12_ROOT_PARAMETER
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &range
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
                    },
                    D3D12_ROOT_PARAMETER
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
                    },
                    D3D12_ROOT_PARAMETER
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor
                        {
                            .ShaderRegister = 1,
                            .RegisterSpace = 0
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
                    }
                });

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc
            {
                .NumParameters = static_cast<UINT>(rootSignatureParams.size()),
                .pParameters = rootSignatureParams.data(),
                .NumStaticSamplers = 0,
                .pStaticSamplers = nullptr,
                .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            };

            using Microsoft::WRL::ComPtr;

            ComPtr<ID3DBlob> signatureBlob;
            ComPtr<ID3DBlob> eBlob;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &eBlob);
            m_spriteRootSignature = backend.GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize()).GetValue();

            ComPtr<ID3DBlob> vertexBlob;
            ComPtr<ID3DBlob> errorBlob;
            hr = D3DCompileFromFile(L"Resources/Shaders/2DVertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexBlob, &errorBlob);
            if(FAILED(hr))
            {
                char* message = static_cast<char*>(errorBlob->GetBufferPointer());
                std::unique_ptr<wchar_t[]> messageT = std::make_unique<wchar_t[]>(errorBlob->GetBufferSize());
                MultiByteToWideChar(CP_ACP, MB_COMPOSITE, message, static_cast<int>(errorBlob->GetBufferSize()), messageT.get(), static_cast<int>(errorBlob->GetBufferSize()));
                //MessageBox(handle, messageT.get(), L"Error", MB_OK);
                return;
            }
            D3D12_SHADER_BYTECODE vertexByteCode{};
            vertexByteCode.BytecodeLength = vertexBlob->GetBufferSize();
            vertexByteCode.pShaderBytecode = vertexBlob->GetBufferPointer();

            ComPtr<ID3DBlob> pixelBlob;
            hr = D3DCompileFromFile(L"Resources/Shaders/2DPixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBlob, nullptr);
            if(FAILED(hr))
                return;

            D3D12_SHADER_BYTECODE pixelByteCode{};
            pixelByteCode.BytecodeLength = pixelBlob->GetBufferSize();
            pixelByteCode.pShaderBytecode = pixelBlob->GetBufferPointer();
            std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout
            {
                D3D12_INPUT_ELEMENT_DESC
                {
                    .SemanticName = "POSITION",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 0,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0
                },
                D3D12_INPUT_ELEMENT_DESC
                {
                    .SemanticName = "TEXCOORD",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0
                }
            };

            D3D12_INPUT_LAYOUT_DESC layoutDesc
            {
                .pInputElementDescs = inputLayout.data(),
                .NumElements = static_cast<UINT>(inputLayout.size())
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineState
            {
                .pRootSignature = m_spriteRootSignature.Get(),
                .VS = vertexByteCode,
                .PS = pixelByteCode,
                .BlendState = D3D12_BLEND_DESC
                {
                    .AlphaToCoverageEnable = false,
                    .IndependentBlendEnable = false
                },
                .SampleMask = 0xff'ff'ff'ff,
                .RasterizerState = D3D12_RASTERIZER_DESC
                {
                    .FillMode = D3D12_FILL_MODE_SOLID,
                    .CullMode = D3D12_CULL_MODE_NONE,
                    .FrontCounterClockwise = true,
                    .DepthBias = 0,
                    .DepthBiasClamp = 0,
                    .SlopeScaledDepthBias = 0,
                    .DepthClipEnable = true,
                    .MultisampleEnable = false,
                    .AntialiasedLineEnable = false,
                    .ForcedSampleCount = 0,
                    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
                },
                .InputLayout = layoutDesc,
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .SampleDesc = {.Count = 1, .Quality = 0}
            };

            graphicsPipelineState.BlendState.RenderTarget[0] = D3D12::Blending::defaultTransparency;

            graphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

            m_spritePipeline = backend.GetDevice()->CreateGraphicsPipelineState(graphicsPipelineState).GetValue();

            m_spriteSampler = backend.GetDevice()->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0).GetValue();            
            D3D12_SAMPLER_DESC samplerDesc
            {
                .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .MipLODBias = 0,
                .MaxAnisotropy = 16,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER,
                .BorderColor = { 1, 1, 1, 1 },
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };
            backend.GetDevice()->CreateSampler(samplerDesc, m_spriteSampler->GetCPUDescriptorHandleForHeapStart());
        }

    public:
        Handle<Sprite> CreateSprite(Texture texture, Math::Types::Vector3f position = {}, Math::Types::Vector3f scale = Math::Types::Scalar(1));
        void SetDefaultTexture(Texture texture)
        {
            m_defaultTexture = texture;
        }

    private:
        void Destroy(Managed<Sprite>* sprite)
        {
            m_sprites.erase(std::find_if(m_sprites.begin(), m_sprites.end(), [=](const std::unique_ptr<Managed<Sprite>>& comp) { return comp.get() == sprite; }));
        }

    public:
        void Draw(D3D12::Backend& backend, TypedD3D::D3D12::CommandList::Direct5 commandList, D3D12::ConstantBuffer& constantBuffer);
    };

    template<>
    struct SpriteRenderer::Managed<Sprite>
    {
        Sprite sprite;
        UINT64 textureHeapOffset = 0;
    };

    template<>
    class SpriteRenderer::Handle<Sprite> : private std::unique_ptr<SpriteRenderer::Managed<Sprite>, SpriteRenderer::Deleter<SpriteRenderer::Managed<Sprite>>>
    {
    private:
        using Base = std::unique_ptr<SpriteRenderer::Managed<Sprite>, SpriteRenderer::Deleter<SpriteRenderer::Managed<Sprite>>>;

    public:
        using Base::Base;
        using Base::element_type;
        using Base::deleter_type;
        using Base::pointer;
        using Base::operator=;
        using Base::swap;

    public:
        void SetTexture(Texture texture)
        {
            object().sprite.texture = (texture.resource != nullptr) ? texture : renderer().m_defaultTexture;
        }

        void SetPosition(Math::Types::Vector3f position)
        {
            object().sprite.position = position;
        }

        void SetScale(Math::Types::Vector3f scale)
        {
            object().sprite.scale = scale;
        }

    private:
        Managed<Sprite>& object() { return *get(); }
        SpriteRenderer& renderer() { return *get_deleter().renderer; }
    };

    SpriteRenderer::Handle<Sprite> SpriteRenderer::CreateSprite(Texture texture, Math::Types::Vector3f position, Math::Types::Vector3f scale)
    {
        m_sprites.push_back(std::make_unique<Managed<Sprite>>());
        m_sprites.back()->sprite.texture = (texture.resource != nullptr) ? texture : m_defaultTexture;
        m_sprites.back()->sprite.position = position;
        m_sprites.back()->sprite.scale = scale;

        return Handle<Sprite>(m_sprites.back().get(), Deleter<Managed<Sprite>>{.renderer = this });
    }

    void SpriteRenderer::Draw(D3D12::Backend& backend, TypedD3D::D3D12::CommandList::Direct5 commandList, D3D12::ConstantBuffer& constantBuffer)
    {
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView
        {
            .BufferLocation = m_spriteMesh->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(sizeof(Vertex) * 6),
            .StrideInBytes = static_cast<UINT>(sizeof(Vertex))
        };

        commandList->SetPipelineState(m_spritePipeline.Get());
        commandList->SetGraphicsRootSignature(m_spriteRootSignature.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);            
        commandList->IASetVertexBuffers(0, std::span(&vertexBufferView, 1));

        commandList->SetGraphicsRootDescriptorTable(1, m_spriteSampler->GetGPUDescriptorHandleForHeapStart().Data());
        auto GPUTextureHandle = m_textures->GetGPUDescriptorHandleForHeapStart(); 
        UINT64 incrementSize = backend.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for(std::unique_ptr<Managed<Sprite>>& sprite : m_sprites)
        {
            commandList->SetGraphicsRootDescriptorTable(0, GPUTextureHandle.Data());
            commandList->SetGraphicsRootConstantBufferView(3, constantBuffer.emplace_back(Math::Matrix::PositionMatrix(sprite->sprite.position) * Math::Matrix::ScaleMatrix(sprite->sprite.scale)));
            commandList->DrawInstanced(6, 1, 0, 0);
        }
    }

    class Renderer2D
    {
    private:
        gsl::strict_not_null<D3D12::Backend*> m_backend;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;
        D3D12::ConstantBuffer m_constantBuffer;
        SpriteRenderer m_spriteRenderer;
        std::deque<std::pair<UINT64, Microsoft::WRL::ComPtr<ID3D12Resource>>> m_temporaryUploadBuffers;

    public:
        Math::Types::Matrix4x4f projectionMatrix;

    public:
        Renderer2D(D3D12::Backend& backend) :
            m_backend(&backend),
            m_commandList(m_backend->GetDevice()->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>()),
            m_constantBuffer(m_backend->GetDevice(), 1, 0),
            m_spriteRenderer(backend)
        {
            Math::Types::Vector2f windowSize = m_backend->GetWindowSize();
            projectionMatrix = Math::Matrix::OrthographicProjectionLH(Math::Types::Vector2f{ 4 * (windowSize.x() / windowSize.y()), 4 }, 0.0001f, 1000.f);

            m_spriteRenderer.SetDefaultTexture(CreateTexture(L"Resources/Korone_NotLikeThis.png"));
        }


    public:
        void Draw()
        {
            using Microsoft::WRL::ComPtr;
            m_commandList->Reset(m_backend->CreateOrGetAllocator(), nullptr);
            m_constantBuffer.Clear(m_backend->GetCurrentFenceValue());
            while(!m_temporaryUploadBuffers.empty() && m_temporaryUploadBuffers.front().first <= m_backend->GetCurrentFenceValue())
            {
                m_temporaryUploadBuffers.pop_front();
            }

            ComPtr<ID3D12Resource> backBuffer = m_backend->GetBackBufferResource();
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->ClearRenderTargetView(m_backend->GetBackBufferHandle(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }), {});

            TypedD3D::D3D12::DescriptorHandle::CPU_RTV backBufferHandle = m_backend->GetBackBufferHandle();
            m_commandList->ClearRenderTargetView(backBufferHandle, std::to_array({ 0.f, 0.3f, 0.7f, 1.f }), {});
            m_commandList->OMSetRenderTargets(std::span(&backBufferHandle, 1), true, nullptr);

            D3D12_VIEWPORT viewport
            {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = static_cast<float>(m_backend->GetWindowSize().x()),
                .Height = static_cast<float>(m_backend->GetWindowSize().y()),
                .MinDepth = 0,
                .MaxDepth = 1
            };

            D3D12_RECT rect
            {
                .left = 0,
                .top = 0,
                .right = static_cast<LONG>(m_backend->GetWindowSize().x()),
                .bottom = static_cast<LONG>(m_backend->GetWindowSize().y())
            };

            m_commandList->RSSetViewports(std::span(&viewport, 1));
            m_commandList->RSSetScissorRects(std::span(&rect, 1));
            m_commandList->SetGraphicsRootConstantBufferView(2, m_constantBuffer.emplace_back(projectionMatrix));
            m_spriteRenderer.Draw(*m_backend, m_commandList, m_constantBuffer);

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            m_backend->ExecuteCommandLists(std::span(submitList));
            m_backend->SignalQueue();
            m_constantBuffer.Signal(m_backend->GetCurrentFenceValue());
            m_backend->Present();
            m_backend->WaitForCurrentFrame();
        }

        Texture CreateTexture(std::wstring file)
        {
            Texture tex;
            DirectX::ScratchImage image;

            DirectX::LoadFromWICFile(
                L"Resources/o_img.png",
                DirectX::WIC_FLAGS_FORCE_RGB,
                nullptr,
                image);

            DirectX::ScratchImage flippedImage;
            DirectX::FlipRotate(*image.GetImage(0, 0, 0), DirectX::TEX_FR_FLIP_VERTICAL, flippedImage);

            const DirectX::Image& texture = *flippedImage.GetImage(0, 0, 0);

            D3D12_RESOURCE_DESC resourceDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Alignment = 0,
                .Width = static_cast<UINT>(texture.width),
                .Height = static_cast<UINT>(texture.height),
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = texture.format,
                .SampleDesc = {.Count = 1, . Quality = 0},
                .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_NONE
            };

            TypedD3D::D3D12::Device5 device = m_backend->GetDevice();
            tex.resource = device->CreateCommittedResource(
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr).GetValue();

            UINT64 requiredIntermediateSize = GetRequiredIntermediateSize(tex.resource.Get(), 0, 1);

            m_temporaryUploadBuffers.push_back({ m_backend->GetCurrentFenceValue() + 1, device->CreateCommittedResource(
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                CD3DX12_RESOURCE_DESC::Buffer(requiredIntermediateSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr).GetValue() });

            D3D12_SUBRESOURCE_DATA data =
            {
                .pData = texture.pixels,
                .RowPitch = static_cast<LONG_PTR>(texture.rowPitch),
                .SlicePitch = static_cast<LONG_PTR>(texture.slicePitch),
            };

            m_commandList->Reset(m_backend->CreateOrGetAllocator(), nullptr);

            UpdateSubresources(m_commandList.Get(), tex.resource.Get(), m_temporaryUploadBuffers.back().second.Get(), 0, 0, 1, &data);
            std::array barrier = std::to_array(
                {
                    TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                        *tex.resource.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
                });
            m_commandList->ResourceBarrier(std::span(barrier));
            m_commandList->Close();

            std::array submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            m_backend->ExecuteCommandLists(std::span(submitList));

            return tex;
        }

        SpriteRenderer::Handle<Sprite> CreateSprite(Texture texture, Math::Types::Vector3f position, Math::Types::Vector3f scale = Math::Types::Scalar(1))
        {
            return m_spriteRenderer.CreateSprite(texture, position, scale);
        }
    };
}