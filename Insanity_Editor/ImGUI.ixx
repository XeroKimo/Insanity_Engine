module;

#include <SDL2/SDL.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_dx11.h>
#include <imgui_stdlib.h>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <string_view>
#include <functional>
#include <ranges>
#include <optional>

export module InsanityEditor.ImGUI;
import xk.Math;
export import TypedD3D11;

namespace InsanityEditor
{
	export struct ImGuiLifetime
	{
		ImGuiContext* context;
		ImGuiLifetime(SDL_Window* window, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
		{
			IMGUI_CHECKVERSION();
			context = ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			io.ConfigDebugIsDebuggerPresent = true;
			io.ConfigWindowsMoveFromTitleBarOnly = true;

			if (!ImGui_ImplSDL2_InitForD3D(window))
				throw std::exception("Failed to init ImGUI");
			if (!ImGui_ImplDX11_Init(device, deviceContext))
				throw std::exception("Failed to init ImGUI");
		}

		ImGuiLifetime(const ImGuiLifetime&) = delete;
		ImGuiLifetime(ImGuiLifetime&&) noexcept = delete;

		ImGuiLifetime& operator=(const ImGuiLifetime&) = delete;
		ImGuiLifetime& operator=(ImGuiLifetime&&) noexcept = delete;

		~ImGuiLifetime()
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext(context);
		}
	};

	export template<class Func>
	void NewImGuiFrame(TypedD3D::Wrapper<ID3D11DeviceContext> context, TypedD3D::Wrapper<ID3D11RenderTargetView> target, Func func)
	{
		struct RenderGuard
		{
			TypedD3D::Wrapper<ID3D11DeviceContext> context;
			TypedD3D::Wrapper<ID3D11RenderTargetView> target;
			~RenderGuard()
			{
				ImGui::Render();
				ImGui::UpdatePlatformWindows();
				context->OMSetRenderTargets(target, nullptr);
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				ImGui::RenderPlatformWindowsDefault();
			}
		};

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		RenderGuard _{ context, target };

		ImGui::DockSpaceOverViewport();
		func();
	}

	struct WindowGuard
	{
		~WindowGuard()
		{
			ImGui::End();
		}
	};

	export template<class Func>
	void NewWindow(std::string_view name, ImGuiWindowFlags flags, Func func)
	{
		ImGui::Begin(name.data(), nullptr, flags);
		WindowGuard _;
		func();
	}

	export template<class Func>
	void NewWindow(std::string_view name, bool& isOpen, ImGuiWindowFlags flags, Func func)
	{
		if (isOpen)
		{
			ImGui::Begin(name.data(), &isOpen, flags);
			WindowGuard _;
			func();
		}
	}

	template<class Ty>
		requires std::integral<std::remove_cvref_t<Ty>> || std::floating_point<std::remove_cvref_t<Ty>>
	consteval ImGuiDataType_ ToDataType()
	{
		using type = std::remove_cvref_t<Ty>;
		if constexpr (std::same_as<type, bool>)
			return ImGuiDataType_Bool;
		if constexpr (std::same_as<type, signed char>)
			return ImGuiDataType_S8;
		if constexpr (std::same_as<type, char>)
			return ImGuiDataType_S8;
		if constexpr (std::same_as<type, unsigned char>)
			return ImGuiDataType_U8;
		if constexpr (std::same_as<type, std::int16_t>)
			return ImGuiDataType_S16;
		if constexpr (std::same_as<type, std::uint16_t>)
			return ImGuiDataType_U16;
		if constexpr (std::same_as<type, std::int32_t>)
			return ImGuiDataType_S32;
		if constexpr (std::same_as<type, std::uint32_t>)
			return ImGuiDataType_U32;
		if constexpr (std::same_as<type, std::int64_t>)
			return ImGuiDataType_S64;
		if constexpr (std::same_as<type, std::uint64_t>)
			return ImGuiDataType_U64;
		if constexpr (std::same_as<type, float>)
			return ImGuiDataType_Float;
		if constexpr (std::same_as<type, double>)
			return ImGuiDataType_Double;
	}

	static_assert(std::integral<bool>);
	export template<class Ty>
		class OptionalChange
	{
		Ty oldValue{};
		std::optional<Ty> newValue;

	public:
		OptionalChange() = default;
		OptionalChange(std::nullopt_t) {}
		OptionalChange(Ty startingValue) :
			oldValue{ startingValue }
		{

		}

		OptionalChange& operator=(const Ty& value)
		{
			newValue = value;
			return *this;
		}

		OptionalChange& operator=(Ty&& value)
		{
			newValue = std::move(value);
			return *this;
		}

		OptionalChange& operator=(const std::optional<Ty>& value)
		{
			newValue = value;
			return *this;
		}

		OptionalChange& operator=(std::optional<Ty>&& value)
		{
			newValue = std::move(value);
			return *this;
		}

		OptionalChange& operator=(std::nullopt_t)
		{
			newValue = std::nullopt;
			return *this;
		}

	public:
		Ty& OldValue() { return oldValue; }
		const Ty& OldValue() const { return oldValue; }

		Ty& NewValue()&
		{
			return newValue.value();
		}

		const Ty& NewValue() const&
		{
			return newValue.value();
		}

		Ty&& NewValue()&&
		{
			return std::move(newValue).value();
		}

		const Ty&& NewValue() const&&
		{
			return std::move(newValue).value();
		}

		Ty NewValueOrOld() const
		{
			return NewValueOr(oldValue);
		}
		Ty NewValueOr(Ty value) const
		{
			return newValue.value_or(std::move(value));
		}

		bool HasNewValue() const { return newValue; }

		explicit operator bool() const { return newValue.has_value(); }

		Ty* operator->() { return newValue.operator->(); }
		const Ty* operator->() const { return newValue.operator->(); }

		Ty& operator*()& { return *newValue; }
		const Ty& operator*() const& { return *newValue; }

		Ty&& operator*()&& { return *std::move(newValue); }
		const Ty&& operator*() const&& { return *std::move(newValue); }
	};

	export OptionalChange<std::string> EditableField(std::string_view label, std::string data, ImGuiInputTextFlags flags = 0)
	{
		return OptionalChange{ data } = ImGui::InputText(label.data(), &data, flags)
			? std::make_optional(std::move(data))
			: std::nullopt;
	}

	export template<class Ty>
		requires std::integral<Ty> || std::floating_point<Ty>
	OptionalChange<Ty> EditableField(std::string_view label, Ty data, ImGuiInputTextFlags flags = 0)
	{
		return OptionalChange{ data } = ImGui::InputScalar(label.data(), ToDataType<Ty>(), &data, nullptr, nullptr, nullptr, flags)
			? std::make_optional(std::move(data))
			: std::nullopt;
	}

	export template<class Ty>
		requires std::ranges::sized_range<Ty>&& std::ranges::contiguous_range<Ty> && (!std::ranges::view<Ty>)
	OptionalChange<Ty> EditableField(std::string_view label, Ty data, ImGuiInputTextFlags flags = 0)
	{
		return OptionalChange{ data } = ImGui::InputScalarN(label.data(), ToDataType<decltype(*std::ranges::begin(data))>(), std::ranges::data(data), std::ranges::size(data), nullptr, nullptr, nullptr, flags)
			? std::make_optional(std::move(data))
			: std::nullopt;
	}

	export template<class Ty>
		requires std::integral<Ty> || std::floating_point<Ty>
	OptionalChange<Ty> EditableDragField(std::string_view label, Ty data, ImGuiInputTextFlags flags = 0)
	{
		return OptionalChange{ data } = ImGui::DragScalar(label.data(), ToDataType<Ty>(), &data, 1.0f, nullptr, nullptr, nullptr, flags)
			? std::make_optional(std::move(data))
			: std::nullopt;
	}

	export template<class Ty>
		requires std::ranges::sized_range<Ty>&& std::ranges::contiguous_range<Ty> && (!std::ranges::view<Ty>)
	OptionalChange<Ty> EditableDragField(std::string_view label, Ty data, ImGuiInputTextFlags flags = 0)
	{
		return OptionalChange{ data } = ImGui::DragScalarN(label.data(), ToDataType<decltype(*std::ranges::begin(data))>(), std::ranges::data(data), std::ranges::size(data), 1.f, nullptr, nullptr, nullptr, flags)
			? std::make_optional(std::move(data))
			: std::nullopt;
	}
}