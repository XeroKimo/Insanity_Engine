module;
#include <d3d11.h>
#include <imgui.h>
#include <functional>
export module InsanityEditor.SceneView;

import InsanityFramework.RendererDX11;
import InsanityEditor.ImGUI;
import TypedD3D11;
import xk.Math;

namespace InsanityEditor
{
	export class SceneView
	{
	public:
		xk::Math::Vector<float, 3> cameraPos;
		float viewSize = 5;

	private:
		TypedD3D::Wrapper<ID3D11RenderTargetView> renderTarget;
		TypedD3D::Wrapper<ID3D11ShaderResourceView> renderTargetAsShaderResource;
		ImVec2 currentWorkSize{};
		bool showControls = true;

	public:
		void Render(TypedD3D::Wrapper<ID3D11Device> device, std::function<void(InsanityFramework::Camera, TypedD3D::Wrapper<ID3D11RenderTargetView>)> renderSceneFunction)
		{
			NewWindow("Scene", ImGuiWindowFlags_MenuBar, [&]
			{
				ResizeTextures(device);

				ImGui::BeginMenuBar();
				if (ImGui::BeginMenu("Controls"))
				{
					ImGui::MenuItem("Show Control Menu", nullptr, &showControls);
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();


				if (showControls)
				{
					ImGui::BeginChild("Scene controls", {}, ImGuiChildFlags_ResizeX | ImGuiChildFlags_Border);
					ImGui::Text("Scene Controls");
					ImGui::Separator();
					ImGui::Text("Camera");
					cameraPos = EditableField("Position", cameraPos, ImGuiInputTextFlags_EnterReturnsTrue).NewValueOrOld();
					viewSize = EditableField("View Size", viewSize, ImGuiInputTextFlags_EnterReturnsTrue).NewValueOrOld();
					ImGui::EndChild();
					ImGui::SameLine();
				}

				ImGui::BeginChild("Scene view");

				viewSize -= ImGui::GetIO().MouseWheel;
				viewSize = (std::max)(0.05f, viewSize);
				if (ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{					
					cameraPos += xk::Math::Vector<float, 3>{ -ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y, 0.f } * (viewSize / currentWorkSize.y);
				}
				renderSceneFunction(InsanityFramework::Camera{ cameraPos, 0, xk::Math::OrthographicProjectionAspectRatioLH({ currentWorkSize.x, currentWorkSize.y }, viewSize, 0.001f, 100.f) }, renderTarget);
				ImGui::Image(reinterpret_cast<ImTextureID>(renderTargetAsShaderResource.Get()), currentWorkSize);
				ImGui::EndChild();
			});
		}

	private:
		void ResizeTextures(TypedD3D::Wrapper<ID3D11Device> device)
		{
			auto workSize = ImGui::GetWindowContentRegionMax();
			workSize.x -= ImGui::GetWindowContentRegionMin().x;
			workSize.y -= ImGui::GetWindowContentRegionMin().y;
			if (workSize.x != currentWorkSize.x || workSize.y != currentWorkSize.y)
			{
				currentWorkSize = workSize;
				currentWorkSize.x = (std::max)(currentWorkSize.x, 1.f);
				currentWorkSize.y = (std::max)(currentWorkSize.y, 1.f);
				D3D11_TEXTURE2D_DESC desc{};
				//ZeroMemory(&desc, sizeof(desc));
				desc.Width = currentWorkSize.x;
				desc.Height = currentWorkSize.y;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				desc.CPUAccessFlags = 0;

				auto resource = device->CreateTexture2D(desc);
				renderTarget = device->CreateRenderTargetView(resource, {});
				renderTargetAsShaderResource = device->CreateShaderResourceView(resource, {});
			}
		}
	};
}