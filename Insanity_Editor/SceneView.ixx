module;
#include <d3d11.h>
#include <imgui.h>
#include <functional>
#include <iostream>
#include <imguizmo.h>
export module InsanityEditor.SceneView;

import InsanityFramework.RendererDX11;
import InsanityEditor.ImGUI;
import InsanityFramework.TransformationNode;
import InsanityFramework.ECS.Scene;
import TypedD3D11;
import xk.Math;
import InsanityEditor.EditorContext;

namespace InsanityEditor
{
	export class SceneView
	{
	public:
		xk::Math::Vector<float, 3> cameraPos;
		float viewSize = 5;
		xk::Math::Vector<float, 2> worldMousePosition;

		InsanityFramework::TransformNode* node = nullptr;
		bool wasDragging = false;
	private:
		TypedD3D::Wrapper<ID3D11RenderTargetView> renderTarget;
		TypedD3D::Wrapper<ID3D11ShaderResourceView> renderTargetAsShaderResource;
		ImVec2 currentWorkSize{};
		bool showControls = true;

	public:
		void Render(std::function<void(InsanityFramework::Camera, TypedD3D::Wrapper<ID3D11RenderTargetView>)> renderSceneFunction)
		{
			NewWindow({ "Scene" }, ImGuiWindowFlags_MenuBar, [&]
			{

				ImGui::BeginMenuBar();
				ImGui::PushStyleColor(ImGuiCol_Button, showControls ? ImVec4{ 0.f, 0.88f, 0.f, 1.f } : ImVec4{ 0.88f, 0.f, 0.f, 1.f });
				if (ImGui::Button("Controls"))
				{
					showControls = !showControls;
				}
				ImGui::PopStyleColor();
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

				ResizeTextures(EditorContext::GetActiveContext().device);
				viewSize -= ImGui::GetIO().MouseWheel;
				viewSize = (std::max)(0.05f, viewSize);

				InsanityFramework::Camera camera{ cameraPos, 0, xk::Math::OrthographicProjectionAspectRatioLH({ currentWorkSize.x, currentWorkSize.y }, viewSize, 0.001f, 100.f) };
				worldMousePosition = xk::Math::HadamardDivision<float, float, 2>(GetLocalMousePosition(), { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y });
				worldMousePosition *= 2;
				worldMousePosition -= xk::Math::Vector<float, 2>{ 1, 1 };

				auto inversed = xk::Math::Inverse(xk::Math::OrthographicProjectionAspectRatioLH({ currentWorkSize.x, currentWorkSize.y }, viewSize, 0.001f, 100.f)) * xk::Math::Vector<float, 4>{ worldMousePosition.X(), worldMousePosition.Y(), 0, 0 };
				worldMousePosition.X() = inversed.At(0, 0);
				worldMousePosition.Y() = inversed.At(1, 0);
				worldMousePosition += xk::Math::Vector{ cameraPos.X(), cameraPos.Y() };
				bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

				if (ImGui::IsWindowFocused())
				{					
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						bool selected = false;
						for (InsanityFramework::TransformNode* transform : InsanityFramework::Scene::GetObjects<InsanityFramework::TransformNode>())
						{
							if (xk::Math::MagnitudeSquared(xk::Math::Vector<float, 2>(transform->WorldTransform().Position().Get().X(), transform->WorldTransform().Position().Get().Y()) - worldMousePosition) <= 1.f)
							{
								std::cout << typeid(*transform).name() << "\n";
								node = transform;
								selected = true;
							}
						}
						if (!selected)
							node = nullptr;
					}
					if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
					{
						auto delta = xk::Math::Vector<float, 3>{ -ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y, 0.f } *(viewSize / currentWorkSize.y);
						cameraPos += delta;
						wasDragging = wasDragging || xk::Math::MagnitudeSquared(delta) != 0;
					}
					else
					{
						wasDragging = false;
					}
				}
				renderSceneFunction(camera, renderTarget);
				ImGui::Image(reinterpret_cast<ImTextureID>(renderTargetAsShaderResource.Get()), currentWorkSize);

				ImGuizmo::SetOrthographic(true);
				ImGuizmo::SetDrawlist();
				float windowWidth = (float)ImGui::GetWindowWidth();
				float windowHeight = (float)ImGui::GetWindowHeight();
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
				static auto test = xk::Math::Matrix<float, 4, 4>::Identity();
				//ImGuizmo::DrawCubes(camera.viewTransform._values.data(), camera.perspectiveTransform._values.data(), test._values.data(), 1);
				ImGuizmo::DrawGrid(camera.viewTransform._values.data(), camera.perspectiveTransform._values.data(), xk::Math::Quaternion<float>{{ 1.f, 0.f, 0.f}, xk::Math::Degree{ 90.f }}.ToRotationMatrix()._values.data(), 100);

				ImGuizmo::Enable(node);
				if (node)
				{
					auto transform = node->WorldTransform().Get().ToMatrix();
					ImGuizmo::Manipulate(camera.viewTransform._values.data(), camera.perspectiveTransform._values.data(), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, transform._values.data());
					node->WorldTransform().Set(InsanityFramework::Transform::FromMatrix(transform));
				}
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