module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <filesystem>
#include <memory>
#include <utility>
#include <wrl/client.h>

export module InsanityEngine;
export import :Renderer;
export import TypedD3D11;
export import TypedDXGI;
export import SDL2pp;

#undef CreateWindow

namespace InsanityEngine
{
	struct EngineConfig
	{
		std::filesystem::path relativeEngineAssetPath = std::filesystem::path{ "../Insanity_Engine/Insanity_Framework/" };
		bool enableDebugRendering = true;
	};

	struct DebugDevice
	{
		Microsoft::WRL::ComPtr<ID3D11Debug> debugDevice;

		DebugDevice();
		~DebugDevice()
		{
			if (debugDevice)
				debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_SUMMARY | D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		}
	};	
	
	struct SDLLifetime
	{
		~SDLLifetime()
		{
			SDL_Quit();
		}
	};

	export const EngineConfig config;
	export auto device = TypedD3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, config.enableDebugRendering ? D3D11_CREATE_DEVICE_DEBUG : 0, D3D_FEATURE_LEVEL_11_1, D3D11_SDK_VERSION);
	export DebugDevice debugDevice;
	const SDLLifetime lifetime;
	export SDL2pp::unique_ptr<SDL2pp::Window> window = SDL2pp::CreateWindow("On My Way Home", { 1280, 720 }, {});
	export TypedDXGI::Wrapper<IDXGISwapChain1> swapChain = []
	{
		TypedDXGI::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory1<IDXGIFactory2>();
		return factory->CreateSwapChainForHwnd<IDXGISwapChain1>(
			InsanityEngine::Renderer::GetDevice(),
			window->GetInternalHandle(),
			DXGI_SWAP_CHAIN_DESC1
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc
				{
					.Count = 1
				},
				.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = 2,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			},
			nullptr,
			nullptr);
	}();
	export TypedD3D11::Wrapper<ID3D11RenderTargetView> backBuffer = InsanityEngine::Renderer::GetDevice()->CreateRenderTargetView(swapChain->GetBuffer<ID3D11Resource>(0));

	export Renderer::SpritePipeline spritePipeline;
	export Renderer::DebugPipeline debugPipeline;

	DebugDevice::DebugDevice() :
		debugDevice{ TypedD3D::Cast<ID3D11Debug>(device.first.AsComPtr()) }
	{
	}
};