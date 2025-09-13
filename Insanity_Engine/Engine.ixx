module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <filesystem>
#include <memory>
#include <utility>
#include <wrl/client.h>
#include <SDL2/SDL.h>
#include <type_traits>
#include <concepts>
#include <chrono>

export module InsanityEngine;
export import :Renderer;
export import :Controller;
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

    Key MapSDLKeycodeToKey(SDL_Keycode sdlKey) {
        switch(sdlKey) {
            // Alphanumeric
        case SDLK_a: return Key::A;
        case SDLK_b: return Key::B;
        case SDLK_c: return Key::C;
        case SDLK_d: return Key::D;
        case SDLK_e: return Key::E;
        case SDLK_f: return Key::F;
        case SDLK_g: return Key::G;
        case SDLK_h: return Key::H;
        case SDLK_i: return Key::I;
        case SDLK_j: return Key::J;
        case SDLK_k: return Key::K;
        case SDLK_l: return Key::L;
        case SDLK_m: return Key::M;
        case SDLK_n: return Key::N;
        case SDLK_o: return Key::O;
        case SDLK_p: return Key::P;
        case SDLK_q: return Key::Q;
        case SDLK_r: return Key::R;
        case SDLK_s: return Key::S;
        case SDLK_t: return Key::T;
        case SDLK_u: return Key::U;
        case SDLK_v: return Key::V;
        case SDLK_w: return Key::W;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;

        case SDLK_0: return Key::Num0;
        case SDLK_1: return Key::Num1;
        case SDLK_2: return Key::Num2;
        case SDLK_3: return Key::Num3;
        case SDLK_4: return Key::Num4;
        case SDLK_5: return Key::Num5;
        case SDLK_6: return Key::Num6;
        case SDLK_7: return Key::Num7;
        case SDLK_8: return Key::Num8;
        case SDLK_9: return Key::Num9;

            // Function keys
        case SDLK_F1: return Key::F1;
        case SDLK_F2: return Key::F2;
        case SDLK_F3: return Key::F3;
        case SDLK_F4: return Key::F4;
        case SDLK_F5: return Key::F5;
        case SDLK_F6: return Key::F6;
        case SDLK_F7: return Key::F7;
        case SDLK_F8: return Key::F8;
        case SDLK_F9: return Key::F9;
        case SDLK_F10: return Key::F10;
        case SDLK_F11: return Key::F11;
        case SDLK_F12: return Key::F12;

            // Modifier keys
        case SDLK_LSHIFT: return Key::ShiftLeft;
        case SDLK_RSHIFT: return Key::ShiftRight;
        case SDLK_LCTRL: return Key::CtrlLeft;
        case SDLK_RCTRL: return Key::CtrlRight;
        case SDLK_LALT: return Key::AltLeft;
        case SDLK_RALT: return Key::AltRight;
        case SDLK_LGUI: return Key::MetaLeft;
        case SDLK_RGUI: return Key::MetaRight;

            // Navigation
        case SDLK_UP: return Key::ArrowUp;
        case SDLK_DOWN: return Key::ArrowDown;
        case SDLK_LEFT: return Key::ArrowLeft;
        case SDLK_RIGHT: return Key::ArrowRight;
        case SDLK_HOME: return Key::Home;
        case SDLK_END: return Key::End;
        case SDLK_PAGEUP: return Key::PageUp;
        case SDLK_PAGEDOWN: return Key::PageDown;

            // Control keys
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_TAB: return Key::Tab;
        case SDLK_CAPSLOCK: return Key::CapsLock;
        case SDLK_RETURN: return Key::Enter;
        case SDLK_SPACE: return Key::Space;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_INSERT: return Key::Insert;
        case SDLK_DELETE: return Key::Delete;

            // Symbols
        case SDLK_BACKQUOTE: return Key::Tilde;
        case SDLK_MINUS: return Key::Minus;
        case SDLK_EQUALS: return Key::Equal;
        case SDLK_LEFTBRACKET: return Key::LeftBracket;
        case SDLK_RIGHTBRACKET: return Key::RightBracket;
        case SDLK_BACKSLASH: return Key::Backslash;
        case SDLK_SEMICOLON: return Key::Semicolon;
        case SDLK_QUOTE: return Key::Apostrophe;
        case SDLK_COMMA: return Key::Comma;
        case SDLK_PERIOD: return Key::Period;
        case SDLK_SLASH: return Key::Slash;

            // Numpad
        case SDLK_KP_0: return Key::NumPad0;
        case SDLK_KP_1: return Key::NumPad1;
        case SDLK_KP_2: return Key::NumPad2;
        case SDLK_KP_3: return Key::NumPad3;
        case SDLK_KP_4: return Key::NumPad4;
        case SDLK_KP_5: return Key::NumPad5;
        case SDLK_KP_6: return Key::NumPad6;
        case SDLK_KP_7: return Key::NumPad7;
        case SDLK_KP_8: return Key::NumPad8;
        case SDLK_KP_9: return Key::NumPad9;
        case SDLK_KP_PLUS: return Key::NumPadAdd;
        case SDLK_KP_MINUS: return Key::NumPadSubtract;
        case SDLK_KP_MULTIPLY: return Key::NumPadMultiply;
        case SDLK_KP_DIVIDE: return Key::NumPadDivide;
        case SDLK_KP_PERIOD: return Key::NumPadDecimal;
        case SDLK_KP_ENTER: return Key::NumPadEnter;

            // Lock keys
        case SDLK_NUMLOCKCLEAR: return Key::NumLock;
        case SDLK_SCROLLLOCK: return Key::ScrollLock;

            // Media keys
        case SDLK_AUDIOPLAY: return Key::PlayPause;
        case SDLK_AUDIOSTOP: return Key::Stop;
        case SDLK_AUDIONEXT: return Key::NextTrack;
        case SDLK_AUDIOPREV: return Key::PrevTrack;
        case SDLK_VOLUMEUP: return Key::VolumeUp;
        case SDLK_VOLUMEDOWN: return Key::VolumeDown;
        case SDLK_MUTE: return Key::Mute;

            // System keys
        case SDLK_PRINTSCREEN: return Key::PrintScreen;
        case SDLK_PAUSE: return Key::Pause;
        case SDLK_MENU: return Key::Menu;

        default: return Key::Unknown;
        }
    }

	export bool HandleEvent(const SDL2pp::Event& event)
	{
		switch(event.type)
		{
		case SDL_KEYDOWN:
            Controller::Set(MapSDLKeycodeToKey(event.key.keysym.sym));
		    return true;
		case SDL_KEYUP:
            Controller::Reset(MapSDLKeycodeToKey(event.key.keysym.sym));
		    return true;

		}

		return false;
	}

    template<class Func, class R, class... Args>
    concept InvocableR = std::is_invocable_r_v<R, Func, Args...>;
    export enum class EventResult
    {
        Consume,
        Passthrough,
    };

    template<std::invocable<std::chrono::nanoseconds> Func>
    void Update(std::chrono::steady_clock::time_point& previousPoint, Func func)
    {
        auto now = std::chrono::steady_clock::now();
        func(now - previousPoint);
        previousPoint = now;
    }

    export class DefaultRenderFunction
    {
        Renderer::Camera& camera;
    public:
        DefaultRenderFunction(Renderer::Camera& camera) : camera{ camera }
        {

        }

        void operator()(auto&)
        {
            InsanityEngine::Renderer::DrawScene(InsanityEngine::backBuffer, camera);
        }
    };

    export template<class GameSystemsType, 
        InvocableR<GameSystemsType> GameSystemFactoryFunc, 
        std::invocable<GameSystemsType&> GameMainFunc,
        InvocableR<EventResult, GameSystemsType&, SDL2pp::Event> GameInputFunc,
        std::invocable<GameSystemsType&, std::chrono::nanoseconds> GameUpdateFunc,
        std::invocable<GameSystemsType&> GameRenderFunc = DefaultRenderFunction>
    int DefaultMain(GameSystemFactoryFunc initFunc, GameMainFunc mainFunc, GameInputFunc inputFunc, GameUpdateFunc updateFunc, GameRenderFunc renderFunc)
    {
        GameSystemsType gameSystems = initFunc();
        mainFunc(gameSystems);
        SDL2pp::Event event;
        auto previous = std::chrono::steady_clock::now();
        while(true)
        {
            if(SDL2pp::PollEvent(event))
            {
                if(event.type == SDL2pp::EventType::SDL_QUIT)
                    return 0;

                if(HandleEvent(event)) {}
                else if(inputFunc(gameSystems, event) == EventResult::Consume) {}
            }
            else
            {
                Update(previous, [&](std::chrono::nanoseconds delta)
                {
                    updateFunc(gameSystems, delta);
                });

                Renderer::GetDeviceContext()->ClearRenderTargetView(backBuffer, { 0.f, 0.3f, 0.87f, 1.f });
                renderFunc(gameSystems);
                swapChain->Present(0, 0);
                Controller::ClearBuffer();
            }
        }

        return 0;
    }
};