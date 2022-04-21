#include "Application.h"
#include "../Rendering/Window.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <utility>

#include "imgui.h"
#include <d3dcompiler.h>
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl.h"
#include <concepts>
#include <filesystem>

namespace InsanityEngine::Application
{

    template<std::invocable Invocable>
    void GuiWindow(std::string_view name, bool* isUncollapsedOrVisible, ImGuiWindowFlags flags, Invocable invocable)
    {
        if(ImGui::Begin(name.data(), isUncollapsedOrVisible, flags))
            std::invoke(invocable);
        ImGui::End();
    }

    class FileSystem
    {
    private:
        bool m_show = true;

    public:
        void Show()
        {
            m_show = true;
        }
        void Hide()
        {
            m_show = false;
        }

        void Draw()
        {
            GuiWindow("File System", &m_show, ImGuiWindowFlags_NoCollapse, [&]()
            {
                ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
                ImGui::BeginChild("Content Tree", ImVec2{ 150, 00 }, true, flags);


                for(auto& file : std::filesystem::directory_iterator("Resources"))
                {
                    if(file.is_directory())
                    {
                        std::string path = file.path().string();

                        if(ImGui::TreeNode(path.c_str()))
                        {
                            ImGui::Text("Hello world");
                            ImGui::TreePop();
                        }
                    }
                }

                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::BeginChild("Content", ImVec2{ 00, 00 }, true);

                for(auto& file : std::filesystem::directory_iterator("Resources"))
                {
                    std::string path = file.path().string();
                    ImGui::Text("%s", path.c_str());
                }
                ImGui::EndChild();
            });
        }
    };

    class ImGuiDrawer
    {
        gsl::strict_not_null<Rendering::Window::DirectX12*> m_renderer;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

        TypedD3D::D3D12::DescriptorHeap::CBV_SRV_UAV m_imGuiDescriptorHeap;
        FileSystem m_fileSystem;

        bool m_showWindow2 = true;
    public:
        ImGuiDrawer(Rendering::Window::DirectX12& renderer) :
            m_renderer(&renderer),
            m_commandList(m_renderer->GetDevice()->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE).GetValue().As<TypedD3D::D3D12::CommandList::Direct5>()),
            m_imGuiDescriptorHeap(m_renderer->GetDevice()->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>(1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0).GetValue())
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

            ImGui_ImplSDL2_InitForD3D(&m_renderer->GetWindow().GetWindow());
            ImGui_ImplDX12_Init(
                m_renderer->GetDevice().Get(),
                m_renderer->GetSwapChainDescription().BufferCount,
                m_renderer->GetSwapChainDescription().Format,
                m_imGuiDescriptorHeap.Get(),
                m_imGuiDescriptorHeap->GetCPUDescriptorHandleForHeapStart().Data(),
                m_imGuiDescriptorHeap->GetGPUDescriptorHandleForHeapStart().Data());
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

        void Draw()
        {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame(&m_renderer->GetWindow().GetWindow());
            ImGui::NewFrame();

            ImGuiViewport* viewport = ImGui::GetMainViewport();        
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            //ImGui::DockSpaceOverViewport();
            static bool open = true;
            GuiWindow("Main DockSpace", &open, window_flags, [&]()
            {
                ImGuiIO& io = ImGui::GetIO();
                if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
                {
                    ImGuiID dockspace_id = ImGui::GetID("Dock Space");
                    ImGui::DockSpace(viewport->ID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
                    ImGui::PopStyleVar(3);
                }
            });

            static bool show_demo_window = true;
            static bool show_another_window = true;
            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            if(show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                if(show_demo_window)
                {

                    GuiWindow("Test", &show_demo_window, ImGuiWindowFlags_None, [&]()
                    {

                    });
                }
                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if(show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if(ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }

            m_fileSystem.Draw();

            // Rendering
            ImGui::Render();

            using Microsoft::WRL::ComPtr;

            m_commandList->Reset(m_renderer->CreateOrGetAllocator(), nullptr);

            ComPtr<ID3D12Resource> backBuffer = m_renderer->GetBackBufferResource();
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_commandList->ResourceBarrier(std::span(&barrier, 1));

            TypedD3D::D3D12::DescriptorHandle::CPU_RTV backBufferHandle = m_renderer->GetBackBufferHandle();
            m_commandList->ClearRenderTargetView(backBufferHandle, std::to_array({ clear_color.x, clear_color.y, clear_color.z, clear_color.w }), {});
            m_commandList->OMSetRenderTargets(std::span(&backBufferHandle, 1), true, nullptr);
            m_commandList->SetDescriptorHeaps(m_imGuiDescriptorHeap);

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            m_commandList->ResourceBarrier(std::span(&barrier, 1));
            m_commandList->Close();

            auto executingCommandList = std::to_array<TypedD3D::D3D12::CommandList::Direct>({ m_commandList });
            m_renderer->ExecuteCommandLists(std::span(executingCommandList));

            ImGuiIO& io = ImGui::GetIO();
            if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            m_renderer->SignalQueue();
            m_renderer->Present();
            m_renderer->WaitForCurrentFrame();
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
            Rendering::Window window = Rendering::Window::Create<ImGuiDrawer>(
                        settings.applicationName,
                        { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED },
                        settings.windowResolution,
                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE,
                        *factory.Get(),
                        device);
            SDL_Event event;
            bool running = true;
            while(running)
            {
                if(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_EventType::SDL_QUIT)
                        running = false;

                    ImGui_ImplSDL2_ProcessEvent(&event);
                    window.HandleEvent(event);

                    switch(event.type)
                    {
                    case SDL_EventType::SDL_WINDOWEVENT:
                        if(event.window.windowID == SDL_GetWindowID(&window.GetWindow()))
                        {
                            if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                                running = false;
                        }
                        break;
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
