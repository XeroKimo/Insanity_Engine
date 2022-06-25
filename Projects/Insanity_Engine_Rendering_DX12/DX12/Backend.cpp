#include "Backend.h"
#include "d3dx12.h"
#include "DX12/VertexFormats.h"
#include <d3dcompiler.h>

namespace InsanityEngine::Rendering::D3D12
{
    Backend::Backend(const BackendInitParams& params) :
        m_device(params.device),
        m_mainQueue(m_device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(
            D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0).value()),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain3>(
            *params.factory.Get(),
            *m_mainQueue.get().Get(),
            params.windowHandle,
            params.swapChainFormat,
            params.bufferCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).value()),
        m_swapChainDescriptorHeap(m_device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE>(params.bufferCount, 0).value()),
        m_mainFence(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE).value()),
        m_rtvHandleIncrement(m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
    {
        CreateRenderTargets();
        m_frameFenceValues.reserve(5);
        m_frameFenceValues.resize(params.bufferCount);
        m_frameResources.reserve(5);
        m_frameResources.resize(params.bufferCount);

        for(UINT i = 0; i < params.bufferCount; i++)
        {
            m_frameResources[i] = m_swapChain->GetBuffer<ID3D12Resource>(i).value();
        }
    }

    Backend::~Backend()
    {
        Flush();
        SetFullscreen(false);
    }

    void Backend::ResizeBuffers(Math::Types::Vector2ui size)
    {
        Reset();

        m_frameResources.clear();
        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();
        m_swapChain->ResizeBuffers(desc.BufferCount, size.x(), size.y(), desc.Format, desc.Flags);
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        for(UINT i = 0; i < desc.BufferCount; i++)
        {
            m_frameResources.push_back(m_swapChain->GetBuffer<ID3D12Resource>(i).value());
        }
        CreateRenderTargets();
    }

    void Backend::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        Reset();

        m_frameResources.clear();
        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(desc.BufferCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();


        for(UINT i = 0; i < desc.BufferCount; i++)
        {
            m_frameResources.push_back(m_swapChain->GetBuffer<ID3D12Resource>(i).value());
        }
        CreateRenderTargets();
    }

    void Backend::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = m_swapChain->GetDesc1();
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = swapChainDesc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(modeDesc);
    }

    bool Backend::IsFullscreen() const
    {
        return m_swapChain->GetFullscreenState().first;
    }

    void Backend::Reset()
    {
        Flush();

        TypedD3D::Helpers::D3D12::ResetFence(*m_mainFence.get().Get());
        m_highestFrameFenceValue = 0;
        for(UINT64& fenceValue : m_frameFenceValues)
        {
            fenceValue = 0;
        }
    }

    void Backend::CreateRenderTargets()
    {
        DXGI_SWAP_CHAIN_DESC1 desc = m_swapChain->GetDesc1();

        D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Texture2D.PlaneSlice = 0;
        viewDesc.Format = desc.Format;

        if(viewDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
            viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        else if(viewDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
            viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        TypedD3D::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> handle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        UINT64 rtvOffset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for(UINT i = 0; i < desc.BufferCount; i++)
        {
            m_device->CreateRenderTargetView(*m_swapChain->GetBuffer<ID3D12Resource>(i).value().Get(), &viewDesc, handle);
            handle = handle.Offset(rtvOffset);
        }
    }

    using Vertex = Common::VertexFormat::Position::Format;

    DefaultDraw::DefaultDraw(Backend& renderer) :
        m_renderer(&renderer),
        m_commandList(TypedD3D::Cast<ID3D12GraphicsCommandList5>(m_renderer->GetDevice()->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).value())),
        m_allocatorManager(m_renderer->GetDevice(), m_renderer->GetBufferCount())
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc
        {
            .NumParameters = 0,
            .pParameters = nullptr,
            .NumStaticSamplers = 0,
            .pStaticSamplers = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        };
        using Microsoft::WRL::ComPtr;

        ComPtr<ID3DBlob> signatureBlob;
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, nullptr);
        m_rootSignature = m_renderer->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize()).value();

        ComPtr<ID3DBlob> vertexBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(L"Default_Resources/VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexBlob, &errorBlob);
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
        hr = D3DCompileFromFile(L"Default_Resources/PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBlob, nullptr);
        if(FAILED(hr))
            return;

        D3D12_SHADER_BYTECODE pixelByteCode{};
        pixelByteCode.BytecodeLength = pixelBlob->GetBufferSize();
        pixelByteCode.pShaderBytecode = pixelBlob->GetBufferPointer();

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
            .InputLayout = VertexFormat::PositionNormalUV::layout,
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .SampleDesc = {.Count = 1, .Quality = 0}
        };

        graphicsPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        graphicsPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        graphicsPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        graphicsPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        graphicsPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        graphicsPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        graphicsPipelineState.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        graphicsPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        graphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        m_pipelineState = m_renderer->GetDevice()->CreateGraphicsPipelineState(graphicsPipelineState).value();

        auto vertices = std::to_array<Vertex>(
            {
                {{ -0.5f, -0.5f, 0 }},// { Math::Types::Scalar(0) }, { Math::Types::Scalar(0) }},
                {{  0.0f,  0.5f, 0 }},// { Math::Types::Scalar(0) }, { Math::Types::Scalar(0) }},
                {{  0.5f, -0.5f, 0 }},// { Math::Types::Scalar(0) }, { Math::Types::Scalar(0) }},
            });

        D3D12_HEAP_PROPERTIES vertexHeap
        {
            .Type = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 0,
            .VisibleNodeMask = 0
        };

        D3D12_RESOURCE_DESC vertexDesc
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {.Count = 1, . Quality = 0},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE
        };


        m_vertexBuffer = m_renderer->GetDevice()->CreateCommittedResource(
            vertexHeap,
            D3D12_HEAP_FLAG_NONE,
            vertexDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr).value();


        D3D12_HEAP_PROPERTIES uploadProperties
        {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 0,
            .VisibleNodeMask = 0
        };
        ComPtr<ID3D12Resource> vertexUpload = m_renderer->GetDevice()->CreateCommittedResource(
            uploadProperties,
            D3D12_HEAP_FLAG_NONE,
            vertexDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr).value();

        D3D12_SUBRESOURCE_DATA vertexData
        {
            .pData = vertices.data(),
            .RowPitch = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
            .SlicePitch = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
        };

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView
        {
            .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(sizeof(Vertex) * 3),
            .StrideInBytes = static_cast<UINT>(sizeof(Vertex))
        };

        m_mesh.vertexBufferView = vertexBufferView;

        TypedD3D::Helpers::D3D12::TailCPUWait({ *m_renderer->GetFence().get().Get() }, m_renderer->GetCurrentFenceValue(), 
            [&]()
            {
                TypedD3D::Helpers::D3D12::GPUWork(*m_renderer->GetCommandQueue().get().Get(), *m_renderer->GetFence().get().Get(), m_renderer->GetCurrentFenceValue(),
                    [&](TypedD3D::Direct<ID3D12CommandQueue> commandQueue)
                    {
                        TypedD3D::Helpers::D3D12::RecordAndExecute(*m_commandList.Get(), *m_allocatorManager.CreateOrGetAllocator(m_renderer->GetCurrentFrameIndex()).Get(), *commandQueue.Get(),
                            [&](TypedD3D::Direct<ID3D12GraphicsCommandList5> commandList)
                            {
                                UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexUpload.Get(), 0, 0, 1, &vertexData);

                                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                                    m_vertexBuffer.Get(),
                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                                m_commandList->ResourceBarrier(std::span(&barrier, 1));
                            });
                    });
            });
    }

    void DefaultDraw::Draw(const FrameData& frame)
    {
        m_allocatorManager.Flush(frame.currentFrameIndex);

        TypedD3D::Helpers::D3D12::RecordAndExecute(*m_commandList.Get(), *m_allocatorManager.CreateOrGetAllocator(m_renderer->GetCurrentFrameIndex()).Get(), *frame.commandQueue.get().Get(),
            [&](TypedD3D::Direct<ID3D12GraphicsCommandList5> commandList)
            {
                D3D12_RESOURCE_BARRIER beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.currentFrameResource.get().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
                D3D12_RESOURCE_BARRIER endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.currentFrameResource.get().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

                TypedD3D::Helpers::D3D12::ResourceBarrier(*commandList.Get(), std::span{ &beginBarrier, 1 }, std::span{ &endBarrier, 1 },
                    [&](TypedD3D::Direct<ID3D12GraphicsCommandList5> commandList)
                    {
                        commandList->ClearRenderTargetView(frame.currentFrameHandle, std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }), {});
                        commandList->OMSetRenderTargets(std::span(&frame.currentFrameHandle, 1), true, nullptr);

                        D3D12_VIEWPORT viewport
                        {
                            .TopLeftX = 0,
                            .TopLeftY = 0,
                            .Width = static_cast<float>(m_renderer->GetWindowSize().x()),
                            .Height = static_cast<float>(m_renderer->GetWindowSize().y()),
                            .MinDepth = 0,
                            .MaxDepth = 1
                        };

                        D3D12_RECT rect
                        {
                            .left = 0,
                            .top = 0,
                            .right = static_cast<LONG>(m_renderer->GetWindowSize().x()),
                            .bottom = static_cast<LONG>(m_renderer->GetWindowSize().y())
                        };

                        commandList->SetPipelineState(m_pipelineState.Get());
                        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
                        commandList->RSSetViewports(std::span(&viewport, 1));
                        commandList->RSSetScissorRects(std::span(&rect, 1));
                        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        commandList->IASetVertexBuffers(0, std::span(&m_mesh.vertexBufferView, 1));
                        commandList->DrawInstanced(3, 1, 0, 0);
                    });

            });
    }
}