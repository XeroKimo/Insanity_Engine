#include "Application.h"
#include "../Rendering/Window.h"
#include "d3dx12.h"
#include <dxgi1_6.h>

#include "imgui.h"
#include <d3dcompiler.h>
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl.h"

namespace InsanityEngine::Application
{
    class ImGuiDrawer
    {
        TypedD3D::D3D12::Device5 m_device;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        TypedD3D::D3D12::DescriptorHeap::CBV_SRV_UAV m_imGuiDescriptorHeap;

        bool m_showWindow2 = true;
    public:
        ImGuiDrawer(TypedD3D::D3D12::Device5 device) :
            m_device(device),
            m_commandList(m_device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>()),
            m_imGuiDescriptorHeap(m_device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0).GetValue())
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
            //io.ConfigViewportsNoAutoMerge = true;
            //io.ConfigViewportsNoTaskBarIcon = true;
                
            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsClassic();

            // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
            ImGuiStyle& style = ImGui::GetStyle();
            if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }
        }

        ~ImGuiDrawer()
        {
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
        }

    public:

        struct Vertex
        {
            float x;
            float y;
            float z;
        };
        void Initialize(Rendering::Window::DirectX12& renderer)
        {
            ImGui_ImplSDL2_InitForD3D(&renderer.GetWindow().GetWindow());
            ImGui_ImplDX12_Init(
                m_device.Get(), 
                renderer.GetSwapChainDescription().BufferCount, 
                renderer.GetSwapChainDescription().Format, 
                m_imGuiDescriptorHeap.Get(), 
                m_imGuiDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Data(), 
                m_imGuiDescriptorHeap->GetGPUDescriptorHandleForHeapStart().Data());
        }

        void Draw(Rendering::Window::DirectX12& renderer)
        {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame(&renderer.GetWindow().GetWindow());
            ImGui::NewFrame();

            bool show_demo_window = true;
            ImGui::ShowDemoWindow(&show_demo_window);

            if(m_showWindow2)
            {
                ImGui::Begin("Another Window", &m_showWindow2);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if(ImGui::Button("Close Me"))
                    m_showWindow2 = false;
                ImGui::End();
            }
            ImGui::Render();

            using Microsoft::WRL::ComPtr;

            m_commandList->Reset(renderer.CreateOrGetAllocator(), nullptr);

            ComPtr<ID3D12Resource> backBuffer = renderer.GetBackBufferResource();
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));

            TypedD3D::D3D12::DescriptorHandle::CPU_RTV backBufferHandle = renderer.GetBackBufferHandle();
            m_commandList->ClearRenderTargetView(backBufferHandle, std::to_array({ 0.f, 0.3f, 0.7f, 1.f }), {});
            m_commandList->OMSetRenderTargets(std::span(&backBufferHandle, 1), true, nullptr);
            m_commandList->SetDescriptorHeaps(m_imGuiDescriptorHeap);

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto executingCommandList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            renderer.ExecuteCommandLists(std::span(executingCommandList));

            ImGuiIO& io = ImGui::GetIO();
            if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

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
                        Rendering::RendererTag<ImGuiDrawer>(),
                        device };
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
