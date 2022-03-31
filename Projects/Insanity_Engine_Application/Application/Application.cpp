#include "Application.h"
#include "../Rendering/Window.h"
#include "DirectXTex/DirectXTex.h"
#include "d3dx12.h"
#include "Extensions/MatrixExtension.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>

namespace InsanityEngine::Application
{
    static D3D12_RENDER_TARGET_BLEND_DESC defaultOpaque
    {
        .BlendEnable = false,
        .LogicOpEnable = false,
        .SrcBlend = D3D12_BLEND_ONE,
        .DestBlend = D3D12_BLEND_ZERO,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
    };
    static D3D12_RENDER_TARGET_BLEND_DESC defaultTransparency
    {
        .BlendEnable = true,
        .LogicOpEnable = false,
        .SrcBlend = D3D12_BLEND_SRC_ALPHA,
        .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
    };

    struct GPUTransferData
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> destination;
        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        D3D12_RESOURCE_DESC resourceDesc;
        D3D12_SUBRESOURCE_DATA data;
    };

    class ConstantBuffer
    {
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
        char* m_begin;
        char* m_current;
        char* m_end;

    public:
        ConstantBuffer() = default;
        ConstantBuffer(TypedD3D::D3D12::Device device, UINT64 size) :
            m_buffer(device->CreateCommittedResource(
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                CD3DX12_RESOURCE_DESC::Buffer((size + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) & (~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1))),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr).GetValue())
        {
            void* begin;
            m_buffer->Map(0, nullptr, &begin);
            m_current = m_begin = reinterpret_cast<char*>(begin);
            m_end = m_begin + m_buffer->GetDesc().Width + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }

        ConstantBuffer(const ConstantBuffer& other) = delete;
        ConstantBuffer(ConstantBuffer&& other) noexcept = default;

        ~ConstantBuffer()
        {
            if(m_buffer)
                m_buffer->Unmap(0, nullptr);
        }

        ConstantBuffer& operator=(const ConstantBuffer& other) = delete;
        ConstantBuffer& operator=(ConstantBuffer&& other) noexcept
        {
            if(m_buffer)
            {
                m_buffer->Unmap(0, nullptr);
            }

            m_buffer = std::move(other.m_buffer);
            m_begin = std::move(other.m_begin);
            m_current = std::move(other.m_current);
            m_end = std::move(other.m_end);

            return *this;
        }

    public:
        D3D12_GPU_VIRTUAL_ADDRESS push_back(const void* data, UINT64 size)
        {
            std::memcpy(m_current, data, size);
            D3D12_GPU_VIRTUAL_ADDRESS address = m_buffer->GetGPUVirtualAddress() + (m_current - m_begin);
            Advance(m_current, size);
            return address;
        }

        template<class Ty>
        D3D12_GPU_VIRTUAL_ADDRESS emplace_back(const Ty& data)
        {
            return push_back(&data, sizeof(Ty));
        }

        void clear()
        {
            //std::memset(m_begin, 0, m_current - m_begin);
            m_current = m_begin;
        }

        void resize(TypedD3D::D3D12::Device device, UINT64 size)
        {
            if(m_buffer)
            {
                m_buffer->Unmap(0, nullptr);
            }

            m_buffer = device->CreateCommittedResource(
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
                CD3DX12_RESOURCE_DESC::Buffer((size + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) & (~D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr).GetValue();

            void* begin;
            m_buffer->Map(0, nullptr, &begin);
            m_current = m_begin = reinterpret_cast<char*>(begin);
            m_end = m_begin + m_buffer->GetDesc().Width + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }

    private:
        void Advance(char* data, UINT64 size)
        {
            data += (size + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) & (~D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        }
    };

    GPUTransferData CreateTextureTransferData(TypedD3D::D3D12::Device5 device, const DirectX::Image& texture)
    {
        using Microsoft::WRL::ComPtr;
        GPUTransferData transferData;

        transferData.resourceDesc =
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

        transferData.destination = device->CreateCommittedResource(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            transferData.resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr).GetValue();

        UINT64 requiredIntermediateSize = GetRequiredIntermediateSize(transferData.destination.Get(), 0, 1);

        transferData.uploadBuffer = device->CreateCommittedResource(
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_RESOURCE_DESC::Buffer(requiredIntermediateSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr).GetValue();

        transferData.data =
        {
            .pData = texture.pixels,
            .RowPitch = static_cast<LONG_PTR>(texture.rowPitch),
            .SlicePitch = static_cast<LONG_PTR>(texture.slicePitch),
        };

        return transferData;
    }

    template<class VertexType>
    GPUTransferData CreateVerticesTransferData(TypedD3D::D3D12::Device5 device, std::span<VertexType> vertices)
    {
        using Microsoft::WRL::ComPtr;
        GPUTransferData transferData;

        D3D12_HEAP_PROPERTIES vertexHeap
        {
            .Type = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 0,
            .VisibleNodeMask = 0
        };

        transferData.resourceDesc =
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = static_cast<UINT>(vertices.size() * sizeof(VertexType)),
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {.Count = 1, . Quality = 0},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE
        };

        transferData.destination = device->CreateCommittedResource(
            vertexHeap,
            D3D12_HEAP_FLAG_NONE,
            transferData.resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr).GetValue();


        D3D12_HEAP_PROPERTIES uploadProperties
        {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 0,
            .VisibleNodeMask = 0
        };

        transferData.uploadBuffer = device->CreateCommittedResource(
            uploadProperties,
            D3D12_HEAP_FLAG_NONE,
            transferData.resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr).GetValue();

        transferData.data =
        {
            .pData = vertices.data(),
            .RowPitch = static_cast<UINT>(vertices.size() * sizeof(VertexType)),
            .SlicePitch = static_cast<UINT>(vertices.size() * sizeof(VertexType)),
        };

        return transferData;
    }

    struct Vertex
    {
        Math::Types::Vector2f position;
        Math::Types::Vector2f UV;
    };

    struct TicTacToeDraw
    {
        TypedD3D::D3D12::Device5 m_device;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        TypedD3D::D3D12::DescriptorHeap::Sampler m_sampler;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_boardTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_xTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_oTexture;
        TypedD3D::D3D12::DescriptorHeap::CBV_SRV_UAV m_textures;

        std::vector<ConstantBuffer> m_constantBuffer; 
        Math::Types::Matrix4x4f m_projectionMatrix;
        D3D12_GPU_VIRTUAL_ADDRESS m_cameraMatrix;


    public:
        TicTacToeDraw(TypedD3D::D3D12::Device5 device) :
            m_device(device),
            m_commandList(m_device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>())
        {
        }

    public:
        void Initialize(Rendering::Window::DirectX12& renderer)
        {
            for(size_t i = 0; i < renderer.GetSwapChainDescription().BufferCount; i++)
                m_constantBuffer.push_back(ConstantBuffer(m_device, 0));

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
            m_rootSignature = m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize()).GetValue();

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
                .pRootSignature = m_rootSignature.Get(),
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

            graphicsPipelineState.BlendState.RenderTarget[0] = defaultTransparency;

            graphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

            m_pipelineState = m_device->CreateGraphicsPipelineState(graphicsPipelineState).GetValue();

            auto vertices = std::to_array<Vertex>(
                {
                    { { -0.5f, -0.5f }, { 0, 0 } },
                    { { -0.5f, 0.5f }, { 0, 1 } },
                    { { 0.5f, 0.5f }, { 1, 1 } },
                    { { -0.5f, -0.5f }, { 0, 0 } },
                    { { 0.5f, 0.5f }, { 1, 1 } },
                    { { 0.5f, -0.5f }, { 1, 0 } },
                });

            GPUTransferData vertexTransfer = CreateVerticesTransferData<Vertex>(m_device, std::span(vertices));
            m_vertexBuffer = vertexTransfer.destination;

            DirectX::ScratchImage ogImage;

            DirectX::LoadFromWICFile(
                //L"Resources/Korone_NotLikeThis.png", 
                L"Resources/ttt_board.png",
                DirectX::WIC_FLAGS_FORCE_RGB,
                nullptr,
                ogImage);

            DirectX::ScratchImage image;
            DirectX::FlipRotate(*ogImage.GetImage(0, 0, 0), DirectX::TEX_FR_FLIP_VERTICAL, image);

            GPUTransferData textureTransfer = CreateTextureTransferData(m_device, *image.GetImage(0, 0, 0));
            m_boardTexture = textureTransfer.destination;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureTransfer.resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            m_textures = m_device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(3, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0).GetValue();
            auto boardDescriptorHandle = m_textures->GetCPUDescriptorHandleForHeapStart();
            m_device->CreateShaderResourceView(*m_boardTexture.Get(), &srvDesc, boardDescriptorHandle);

            DirectX::ScratchImage ogImageX;

            DirectX::LoadFromWICFile(
                L"Resources/x_img.png",
                DirectX::WIC_FLAGS_FORCE_RGB,
                nullptr,
                ogImageX);

            DirectX::ScratchImage imageX;
            DirectX::FlipRotate(*ogImageX.GetImage(0, 0, 0), DirectX::TEX_FR_FLIP_VERTICAL, imageX);

            GPUTransferData textureTransferX = CreateTextureTransferData(m_device, *imageX.GetImage(0, 0, 0));
            m_xTexture = textureTransferX.destination;
            boardDescriptorHandle.Ptr() += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_device->CreateShaderResourceView(*m_xTexture.Get(), &srvDesc, boardDescriptorHandle);

            DirectX::ScratchImage ogImageO;

            DirectX::LoadFromWICFile(
                L"Resources/o_img.png",
                DirectX::WIC_FLAGS_FORCE_RGB,
                nullptr,
                ogImageO);

            DirectX::ScratchImage imageO;
            DirectX::FlipRotate(*ogImageO.GetImage(0, 0, 0), DirectX::TEX_FR_FLIP_VERTICAL, imageO);

            GPUTransferData textureTransferO = CreateTextureTransferData(m_device, *imageO.GetImage(0, 0, 0));
            m_oTexture = textureTransferO.destination;
            boardDescriptorHandle.Ptr() += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_device->CreateShaderResourceView(*m_oTexture.Get(), &srvDesc, boardDescriptorHandle);

            m_sampler = m_device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>(1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0).GetValue();

            D3D12_SAMPLER_DESC samplerDesc
            {
                .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .MipLODBias = 0,
                .MaxAnisotropy = 0,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER,
                .BorderColor = { 1, 1, 1, 1 },
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };
            m_device->CreateSampler(samplerDesc, m_sampler->GetCPUDescriptorHandleForHeapStart());



            Math::Types::Vector2f windowSize = renderer.GetWindowSize();
            m_projectionMatrix = Math::Matrix::OrthographicProjectionLH(Math::Types::Vector2f{ 5 * (windowSize.x() / windowSize.y()), 5 } , 0.0001f, 1000.f);

            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);
            UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexTransfer.uploadBuffer.Get(), 0, 0, 1, &vertexTransfer.data);
            UpdateSubresources(m_commandList.Get(), m_boardTexture.Get(), textureTransfer.uploadBuffer.Get(), 0, 0, 1, &textureTransfer.data);
            UpdateSubresources(m_commandList.Get(), m_xTexture.Get(), textureTransferX.uploadBuffer.Get(), 0, 0, 1, &textureTransferX.data);
            UpdateSubresources(m_commandList.Get(), m_oTexture.Get(), textureTransferO.uploadBuffer.Get(), 0, 0, 1, &textureTransferO.data);

            std::array barrier = std::to_array(
                {
                    TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                        *m_vertexBuffer.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
                    TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                        *m_boardTexture.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
                    TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                        *m_xTexture.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
                    TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                        *m_oTexture.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
                });
            m_commandList->ResourceBarrier(std::span(barrier));
            m_commandList->Close();

            std::array submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(submitList));
            renderer.SignalQueue();
            renderer.WaitForCurrentFrame();
        }

        void Draw(Rendering::Window::DirectX12& renderer)
        {
            using Microsoft::WRL::ComPtr;
            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);
            m_constantBuffer[renderer.GetCurrentBackBufferIndex()].clear();
            m_cameraMatrix = m_constantBuffer[renderer.GetCurrentBackBufferIndex()].emplace_back(m_projectionMatrix);

            ComPtr<ID3D12Resource> backBuffer = renderer.GetBackBufferResource();
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->ClearRenderTargetView(renderer.GetBackBufferHandle(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }), {});

            TypedD3D::D3D12::DescriptorHandle::CPU_RTV backBufferHandle = renderer.GetBackBufferHandle();
            m_commandList->ClearRenderTargetView(backBufferHandle, std::to_array({ 0.f, 0.3f, 0.7f, 1.f }), {});
            m_commandList->OMSetRenderTargets(std::span(&backBufferHandle, 1), true, nullptr);

            D3D12_VERTEX_BUFFER_VIEW vertexBufferView
            {
                .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<UINT>(sizeof(Vertex) * 6),
                .StrideInBytes = static_cast<UINT>(sizeof(Vertex))
            };


            D3D12_VIEWPORT viewport
            {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = static_cast<float>(renderer.GetWindowSize().x()),
                .Height = static_cast<float>(renderer.GetWindowSize().y()),
                .MinDepth = 0,
                .MaxDepth = 1
            };

            D3D12_RECT rect
            {
                .left = 0,
                .top = 0,
                .right = static_cast<LONG>(renderer.GetWindowSize().x()),
                .bottom = static_cast<LONG>(renderer.GetWindowSize().y())
            };


            m_commandList->SetPipelineState(m_pipelineState.Get());
            m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            m_commandList->RSSetViewports(std::span(&viewport, 1));
            m_commandList->RSSetScissorRects(std::span(&rect, 1));
            m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_commandList->IASetVertexBuffers(0, std::span(&vertexBufferView, 1));
            m_commandList->SetDescriptorHeaps(m_textures, m_sampler);
            auto GPUHandle = m_textures->GetGPUDescriptorHandleForHeapStart();
            m_commandList->SetGraphicsRootDescriptorTable(0, GPUHandle.Data());
            m_commandList->SetGraphicsRootDescriptorTable(1, m_sampler->GetGPUDescriptorHandleForHeapStart().Data());
            m_commandList->SetGraphicsRootConstantBufferView(2, m_cameraMatrix);
            m_commandList->DrawInstanced(6, 1, 0, 0);

            GPUHandle.Ptr() += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_commandList->SetGraphicsRootDescriptorTable(0, GPUHandle.Data());
            m_commandList->DrawInstanced(6, 1, 0, 0);

            GPUHandle.Ptr() += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_commandList->SetGraphicsRootDescriptorTable(0, GPUHandle.Data());
            m_commandList->DrawInstanced(6, 1, 0, 0);

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(submitList));
            renderer.SignalQueue();
            renderer.Present();
            renderer.WaitForCurrentFrame();
        }
    };



    int Run(const Settings& settings)
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<ID3D12DebugDevice2> debugDevice;
        {
            ComPtr<IDXGIFactory7> factory = TypedD3D::Helpers::DXGI::Factory::Create<IDXGIFactory7>(TypedD3D::Helpers::DXGI::Factory::CreationFlags::None).GetValue();
            ComPtr<ID3D12Debug3> debug = TypedD3D::Helpers::D3D12::GetDebugInterface<ID3D12Debug3>().GetValue();
            debug->EnableDebugLayer();
            TypedD3D::D3D12::Device5 device = TypedD3D::D3D12::CreateDevice<TypedD3D::D3D12::Device5>(D3D_FEATURE_LEVEL_12_0, nullptr).GetValue();
            debugDevice = TypedD3D::Helpers::COM::Cast<ID3D12DebugDevice2>(device.GetComPtr());

            Rendering::Window window{
                settings.applicationName,
                { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
                settings.windowResolution,
                SDL_WINDOW_SHOWN,
                *factory.Get(),
                device,
                TicTacToeDraw(device) };

            SDL_Event event;
            while(true)
            {
                if(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_EventType::SDL_QUIT)
                        break;

                    window.HandleEvent(event);

                    switch(event.type)
                    {
                    case SDL_EventType::SDL_KEYDOWN:
                        if(event.key.repeat == 0 && event.key.state == SDL_PRESSED)
                        {
                            if(event.key.keysym.sym == SDL_KeyCode::SDLK_1)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                window.SetFullscreen(false);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_2)
                            {
                                SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                window.SetFullscreen(true);
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_3)
                            {
                                if(!window.IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.SetWindowSize({ 1280, 720 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.SetWindowSize({ 1280, 720 });
                                }
                            }
                            else if(event.key.keysym.sym == SDL_KeyCode::SDLK_4)
                            {
                                if(!window.IsFullscreen())
                                {
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_TRUE);
                                    window.SetWindowSize({ 1600, 900 });
                                    SDL_SetWindowResizable(&window.GetWindow(), SDL_FALSE);
                                }
                                else
                                {
                                    window.SetWindowSize({ 1600, 900 });
                                }
                            }
                        }
                        break;
                    }
                }
                else
                {
                    window.Draw();
                }
            }
        }

        if(debugDevice)
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);

        return 0;
    }
}
