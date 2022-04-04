#include "Window.h"
#include "d3dx12.h"
#include <d3dcompiler.h>

namespace InsanityEngine::Rendering
{
    static constexpr UINT frameCount = 2;

    Window::DirectX11::DirectX11(Window& window, IDXGIFactory2& factory, Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext) :
        m_device(device),
        m_deviceContext(deviceContext),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory,
            *m_device.Get(),
            std::any_cast<HWND>(window.GetWindowHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            frameCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue())
    {
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    Window::DirectX11::~DirectX11()
    {
        m_deviceContext->Flush();
        SetFullscreen(false);
    }

    void Window::DirectX11::ResizeBuffers(Math::Types::Vector2ui size)
    {
        m_deviceContext->Flush();

        m_backBuffer->Release();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->ResizeBuffers(frameCount, size.x(), size.y(), desc.Format, desc.Flags);
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    void Window::DirectX11::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        m_deviceContext->Flush();

        m_backBuffer->Release();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(frameCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        m_device->CreateRenderTargetView(TypedD3D::Helpers::DXGI::SwapChain::GetBuffer<ID3D11Resource>(*m_swapChain.Get(), 0).GetValue().Get(), nullptr, m_backBuffer.GetAddressOf());
    }

    void Window::DirectX11::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = swapChainDesc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(&modeDesc);
    }

    bool Window::DirectX11::IsFullscreen() const
    {
        BOOL isFullscreen;
        m_swapChain->GetFullscreenState(&isFullscreen, nullptr);
        return isFullscreen;
    }

    void Window::DirectX11::Present()
    {
        m_swapChain->Present(1, 0);
    }




    Window::DirectX12::DirectX12(Window& window, IDXGIFactory2& factory, TypedD3D::D3D12::Device5 device) :
        m_window(&window),
        m_device(device),
        m_mainQueue(device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(
            D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0).GetValue()),
        m_swapChain(TypedD3D::Helpers::DXGI::SwapChain::CreateFlipDiscard<IDXGISwapChain4>(
            factory,
            *m_mainQueue.Get(),
            std::any_cast<HWND>(window.GetWindowHandle()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            frameCount,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            false).GetValue()),
        m_swapChainDescriptorHeap(device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>(frameCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0).GetValue()),
        m_mainFence(device->CreateFence(0, D3D12_FENCE_FLAG_NONE).GetValue()),
        m_mainFenceWaitValue(0)
    {
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
        m_frameData.reserve(5);
        m_frameData.resize(frameCount);
    }

    Window::DirectX12::~DirectX12()
    {
        Reset();
        SetFullscreen(false);
    }

    void Window::DirectX12::ResizeBuffers(Math::Types::Vector2ui size)
    {
        Reset();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->ResizeBuffers(frameCount, size.x(), size.y(), desc.Format, desc.Flags);
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*m_device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
    }

    void Window::DirectX12::SetFullscreen(bool fullscreen)
    {
        if(IsFullscreen() == fullscreen)
            return;

        Reset();

        DXGI_SWAP_CHAIN_DESC1 desc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        m_swapChain->SetFullscreenState(fullscreen, nullptr);
        m_swapChain->ResizeBuffers(frameCount, desc.Width, desc.Height, desc.Format, desc.Flags);
        TypedD3D::Helpers::D3D12::CreateSwapChainRenderTargets(*m_device.Get(), *m_swapChain.Get(), *m_swapChainDescriptorHeap.Get());
    }

    void Window::DirectX12::SetWindowSize(Math::Types::Vector2ui size)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = TypedD3D::Helpers::Common::GetDescription(*m_swapChain.Get());
        DXGI_MODE_DESC modeDesc
        {
                .Width = size.x(),
                .Height = size.y(),
                .RefreshRate {},
                .Format = swapChainDesc.Format,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
        };
        HRESULT hr = m_swapChain->ResizeTarget(&modeDesc);
    }

    bool Window::DirectX12::IsFullscreen() const
    {
        BOOL isFullscreen;
        m_swapChain->GetFullscreenState(&isFullscreen, nullptr);
        return isFullscreen;
    }

    void Window::DirectX12::SignalQueue()
    {
        m_frameData[m_swapChain->GetCurrentBackBufferIndex()].fenceWaitValue = m_mainFenceWaitValue = TypedD3D::Helpers::D3D12::SignalFenceGPU(*m_mainQueue.Get(), *m_mainFence.Get(), m_mainFenceWaitValue);
    }

    void Window::DirectX12::WaitForCurrentFrame()
    {
        FrameData& currentFrame = m_frameData[GetCurrentBackBufferIndex()];
        TypedD3D::Helpers::D3D12::StallCPUThread(*m_mainFence.Get(), currentFrame.fenceWaitValue);
        currentFrame.idleAllocatorIndex = 0;
    }

    void Window::DirectX12::Present()
    {
        m_swapChain->Present(1, 0);
    }

    TypedD3D::D3D12::DescriptorHandle::CPU_RTV Window::DirectX12::GetBackBufferHandle()
    {
        UINT stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        TypedD3D::D3D12::DescriptorHandle::CPU_RTV handle = m_swapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handle.Ptr() += stride * m_swapChain->GetCurrentBackBufferIndex();
        return handle;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> Window::DirectX12::GetBackBufferResource()
    {
        return TypedD3D::Helpers::DXGI::SwapChain::GetBuffer(*m_swapChain.Get(), GetCurrentBackBufferIndex()).GetValue();
    }

    TypedD3D::D3D12::CommandAllocator::Direct Window::DirectX12::CreateOrGetAllocator()
    {
        FrameData& currentFrame = m_frameData[GetCurrentBackBufferIndex()];
        TypedD3D::D3D12::CommandAllocator::Direct allocator;
        if(currentFrame.idleAllocatorIndex == currentFrame.allocators.size())
        {
            currentFrame.allocators.push_back(m_device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>().GetValue());
            allocator = currentFrame.allocators.back();
        }
        else
        {
            allocator = currentFrame.allocators[currentFrame.idleAllocatorIndex];
            allocator->Reset();
        }
        ++currentFrame.idleAllocatorIndex;

        return allocator;
    }

    UINT Window::DirectX12::GetCurrentBackBufferIndex() const
    {
        return m_swapChain->GetCurrentBackBufferIndex();
    }

    void Window::DirectX12::Reset()
    {
        TypedD3D::Helpers::D3D12::FlushCommandQueue(*m_mainQueue.Get(), *m_mainFence.Get(), m_mainFenceWaitValue);

        TypedD3D::Helpers::D3D12::ResetFence(*m_mainFence.Get());
        m_mainFenceWaitValue = 0;
        for(FrameData& frame : m_frameData)
        {
            frame.fenceWaitValue = 0;
        }
    }

    void Window::HandleEvent(const SDL_Event& event)
    {
        if(event.type == SDL_WINDOWEVENT)
        {
            if(event.window.windowID != SDL_GetWindowID(m_windowHandle.get()))
                return;

            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                m_backEnd->ResizeBuffers({ event.window.data1, event.window.data2 });
            }
        }
    }


    struct Vertex
    {
        float x;
        float y;
        float z;
    };

    namespace D3D11
    {

            DefaultDraw::DefaultDraw(Microsoft::WRL::ComPtr<ID3D11Device5> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext4> deviceContext) :
                m_device(device),
                m_deviceContext(deviceContext)
            {
            }

            void DefaultDraw::Initialize(Window::DirectX11& renderer)
            {
                using Microsoft::WRL::ComPtr;
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
                m_device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &m_vertexShader);

                ComPtr<ID3DBlob> pixelBlob;
                hr = D3DCompileFromFile(L"Default_Resources/PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBlob, nullptr);
                if(FAILED(hr))
                    return;

                m_device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &m_pixelShader);

                std::array<D3D11_INPUT_ELEMENT_DESC, 1> inputLayout
                {
                    D3D11_INPUT_ELEMENT_DESC
                    {
                        .SemanticName = "POSITION",
                        .SemanticIndex = 0,
                        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                        .InputSlot = 0,
                        .AlignedByteOffset = 0,
                        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                        .InstanceDataStepRate = 0
                    }
                };

                m_device->CreateInputLayout(inputLayout.data(), static_cast<UINT>(inputLayout.size()), vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &m_inputLayout);

                auto vertices = std::to_array<Vertex>(
                    {
                        { -0.5f, -0.5f, 0 },
                        {  0.0f,  0.5f, 0 },
                        {  0.5f, -0.5f, 0 },
                    });

                D3D11_BUFFER_DESC bufferDesc{};
                bufferDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
                bufferDesc.Usage = D3D11_USAGE_DEFAULT;
                bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

                bufferDesc.CPUAccessFlags = 0;
                bufferDesc.MiscFlags = 0;
                bufferDesc.StructureByteStride = 0;

                D3D11_SUBRESOURCE_DATA data;
                data.pSysMem = vertices.data();
                data.SysMemPitch = 0;
                data.SysMemSlicePitch = 0;

                m_device->CreateBuffer(&bufferDesc, &data, &m_vertexBuffer);
            }

            void DefaultDraw::Draw(Window::DirectX11& renderer)
            {
                D3D11_VIEWPORT viewport = {};
                viewport.Width = static_cast<float>(renderer.GetWindowSize().x());
                viewport.Height = static_cast<float>(renderer.GetWindowSize().y());
                viewport.MaxDepth = 1;
                viewport.MinDepth = 0;

                D3D11_RECT rect = {};
                rect.right = renderer.GetWindowSize().x();
                rect.bottom = renderer.GetWindowSize().y();

                m_deviceContext->ClearRenderTargetView(&renderer.GetBackBufferResource(), std::to_array({ 0.0f, 0.3f, 0.7f, 1.0f }).data());
                ID3D11RenderTargetView* targets[] = { &renderer.GetBackBufferResource() };
                m_deviceContext->OMSetRenderTargets(1, targets, nullptr);
                m_deviceContext->RSSetViewports(1, &viewport);
                m_deviceContext->RSSetScissorRects(1, &rect);
                m_deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
                m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
                m_deviceContext->IASetInputLayout(m_inputLayout.Get());
                m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;

                m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
                m_deviceContext->Draw(3, 0);

                renderer.Present();
            }
    }
    namespace D3D12
    {
        DefaultDraw::DefaultDraw(TypedD3D::D3D12::Device5 device) :
            m_device(device),
            m_commandList(device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>())
        {
        }

        void DefaultDraw::Initialize(Window::DirectX12& renderer)
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
            m_rootSignature = m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize()).GetValue();

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
            std::array<D3D12_INPUT_ELEMENT_DESC, 1> inputLayout
            {
                D3D12_INPUT_ELEMENT_DESC
                {
                    .SemanticName = "POSITION",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 0,
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

            graphicsPipelineState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
            graphicsPipelineState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
            graphicsPipelineState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            graphicsPipelineState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            graphicsPipelineState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            graphicsPipelineState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            graphicsPipelineState.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
            graphicsPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            graphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

            m_pipelineState = m_device->CreateGraphicsPipelineState(graphicsPipelineState).GetValue();

            auto vertices = std::to_array<Vertex>(
                {
                    { -0.5f, -0.5f, 0 },
                    {  0.0f,  0.5f, 0 },
                    {  0.5f, -0.5f, 0 },
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


            m_vertexBuffer = m_device->CreateCommittedResource(
                vertexHeap,
                D3D12_HEAP_FLAG_NONE,
                vertexDesc,
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
            ComPtr<ID3D12Resource> vertexUpload = m_device->CreateCommittedResource(
                uploadProperties,
                D3D12_HEAP_FLAG_NONE,
                vertexDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr).GetValue();

            D3D12_SUBRESOURCE_DATA vertexData
            {
                .pData = vertices.data(),
                .RowPitch = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
                .SlicePitch = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
            };

            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);
            UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexUpload.Get(), 0, 0, 1, &vertexData);


            D3D12_RESOURCE_BARRIER barrier = TypedD3D::Helpers::D3D12::ResourceBarrier::Transition(
                *m_vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            std::array submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(submitList));
            renderer.SignalQueue();
            renderer.WaitForCurrentFrame();
        }

        void DefaultDraw::Draw(Window::DirectX12& renderer)
        {
            using Microsoft::WRL::ComPtr;
            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);

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
                .SizeInBytes = static_cast<UINT>(sizeof(Vertex) * 3),
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
            m_commandList->DrawInstanced(3, 1, 0, 0);

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto submitList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(submitList));
            renderer.SignalQueue();
            renderer.Present();
            renderer.WaitForCurrentFrame();
        }
    }

}