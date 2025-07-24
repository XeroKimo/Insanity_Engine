module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3d11sdklayers.h>
#include <span>
#include <concepts>
#include <type_traits>
#include <filesystem>
#include <optional>

export module InsanityFramework.RendererDX11;
export import TypedD3D11;
export import TypedDXGI;
export import xk.Math;
using namespace TypedD3D;

namespace InsanityFramework
{
	export TypedD3D11::Wrapper<ID3D11ShaderResourceView> CreateTexture(std::filesystem::path path, TypedD3D11::Wrapper<ID3D11Device> device);

	export class RendererDX11;

	export std::filesystem::path engineAssetPath = std::filesystem::path{ "../Insanity_Engine/Insanity_Framework/" };

	export template<class Func>
	void UpdateConstantBuffer(TypedD3D::Wrapper<ID3D11DeviceContext> context, TypedD3D::Wrapper<ID3D11Resource> resource, Func func)
	{
		D3D11_MAPPED_SUBRESOURCE data = context->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0);
		func(data);
		context->Unmap(resource, 0);
	}

	export template<std::invocable<D3D11_MAPPED_SUBRESOURCE> Func>
	void UpdateConstantBufferNoOverwrite(TypedD3D::Wrapper<ID3D11DeviceContext> context, TypedD3D::Wrapper<ID3D11Resource> resource, Func func)
	{
		D3D11_MAPPED_SUBRESOURCE data = context->Map(resource, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0);
		func(data);
		context->Unmap(resource, 0);
	}

	class RendererDX11
	{
	private:
		TypedD3D11::Wrapper<ID3D11Device> m_device;
		Microsoft::WRL::ComPtr<ID3D11Debug> m_debugDevice;
		TypedD3D11::Wrapper<ID3D11DeviceContext> m_deviceContext;
		TypedDXGI::Wrapper<IDXGISwapChain1> m_swapChain;
		TypedD3D11::Wrapper<ID3D11RenderTargetView> m_backBuffer;

	public:
		RendererDX11(HWND handle);
		~RendererDX11();

	public:

		void DebugPrintMemory()
		{
			m_debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		}

		TypedD3D11::Wrapper<ID3D11Device> GetDevice() const { return m_device; }
		TypedD3D11::Wrapper<ID3D11DeviceContext> GetDeviceContext() const { return m_deviceContext; }
		TypedD3D11::Wrapper<IDXGISwapChain1> GetSwapChain() const { return m_swapChain; }
		TypedD3D11::Wrapper<ID3D11RenderTargetView> GetSwapChainBackBuffer() { return m_backBuffer; }
	};

	export struct Camera
	{
		xk::Math::Matrix<float, 4, 4> viewPerspectiveTransform;

		Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective);
	};

	export struct SpritePipelineDX11;

	export class SpriteRenderInterfaceDX11
	{
	private:
		TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext;
		SpritePipelineDX11& m_spriteRenderer;

	public:
		SpriteRenderInterfaceDX11(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext, SpritePipelineDX11& spriteRenderer) :
			deviceContext{ deviceContext },
			m_spriteRenderer{ spriteRenderer }
		{
		}

		template<std::invocable Func>
		void CameraPass(const Camera& camera, std::optional<TypedD3D11::Wrapper<ID3D11RenderTargetView>> target, Func func);

		void Draw(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, xk::Math::Aliases::Matrix4x4 transform);
		void DrawMultiple(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, std::span<xk::Math::Aliases::Matrix4x4> transform);
	};

	struct SpritePipelineDX11
	{
		TypedD3D11::Wrapper<ID3D11Buffer> cameraBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> vertexBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> instanceBuffer;
		TypedD3D11::Wrapper<ID3D11RasterizerState> rasterizerState;

		TypedD3D11::Wrapper<ID3D11InputLayout> layout;
		TypedD3D11::Wrapper<ID3D11VertexShader> vertexShader;
		TypedD3D11::Wrapper<ID3D11PixelShader> pixelShader;
		TypedD3D11::Wrapper<ID3D11ShaderResourceView> defaultTexture;
		TypedD3D11::Wrapper<ID3D11DepthStencilState> depthState;
		TypedD3D11::Wrapper<ID3D11SamplerState> pointSampler;
		TypedD3D11::Wrapper<ID3D11BlendState> blendState;

	public:
		static constexpr UINT VSPerFrameCBufferSlot = 0;
		static constexpr UINT VSPerCameraCBufferSlot = 1;
		static constexpr UINT VSPerMaterialCBufferSlot = 2;
		static constexpr UINT VSPerObjectCBufferSlot = 3;


	public:
		SpritePipelineDX11(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);

		void Bind(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);

		template<std::invocable<SpriteRenderInterfaceDX11> Func>
		void Bind(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext, Func func)
		{
			Bind(deviceContext);
			func(MakeRenderInterface(deviceContext));
		}

		SpriteRenderInterfaceDX11 MakeRenderInterface(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext)
		{
			return { deviceContext, *this };
		}
	};

	template<std::invocable Func>
	void SpriteRenderInterfaceDX11::CameraPass(const Camera& camera, std::optional<TypedD3D11::Wrapper<ID3D11RenderTargetView>> target, Func func)
	{
		if (target)
		{
			deviceContext->OMSetRenderTargets(target.value(), nullptr);
			D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(target.value()->GetResource())->GetDesc();
			D3D11_VIEWPORT viewports;
			viewports.TopLeftX = 0;
			viewports.TopLeftY = 0;
			viewports.MinDepth = 0;
			viewports.MaxDepth = 1;
			viewports.Width = static_cast<FLOAT>(desc.Width);
			viewports.Height = static_cast<FLOAT>(desc.Height);
			deviceContext->RSSetViewports(viewports);
		}
		UpdateConstantBuffer(deviceContext, m_spriteRenderer.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
		});
		deviceContext->VSSetConstantBuffers(SpritePipelineDX11::VSPerCameraCBufferSlot, m_spriteRenderer.cameraBuffer);
		func();
	}

	export struct DebugPipelineDX11;

	export class DebugRenderInterfaceDX11
	{
	private:
		TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext;
		DebugPipelineDX11& m_debugRenderer;

	public:
		DebugRenderInterfaceDX11(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext, DebugPipelineDX11& debugRenderer) :
			deviceContext{ deviceContext },
			m_debugRenderer{ debugRenderer }
		{
		}

	public:
		void DrawLine(xk::Math::Vector<float, 3> start, xk::Math::Vector<float, 3> end);
		
		template<size_t Count>
		void DrawLine(std::array<xk::Math::Vector<float, 3>, Count> points)
		{
			DrawLine(std::span{ points });
		}

		void DrawLine(std::span<xk::Math::Vector<float, 3>> points);

		template<size_t Count>
		void DrawConnectedLines(std::array<xk::Math::Vector<float, 3>, Count> points)
		{
			DrawConnectedLines(std::span{ points });
		}
		void DrawConnectedLines(std::span<xk::Math::Vector<float, 3>> points);

		void DrawSquare(xk::Math::Vector<float, 3> center, xk::Math::Vector<float, 3> halfSize);
		void DrawCircle(xk::Math::Vector<float, 3> center, float radius);

		void SetColor(xk::Math::Vector<float, 4> rgba);

		template<std::invocable Func>
		void CameraPass(const Camera& camera, std::optional<TypedD3D11::Wrapper<ID3D11RenderTargetView>> target, Func func);
	};

	struct DebugPipelineDX11
	{
		TypedD3D11::Wrapper<ID3D11Buffer> cameraBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> vertexBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> batchBuffer;
		TypedD3D11::Wrapper<ID3D11InputLayout> layout;
		TypedD3D11::Wrapper<ID3D11VertexShader> vertexShader;
		TypedD3D11::Wrapper<ID3D11PixelShader> pixelShader;

		static constexpr UINT PSPerBatchCBufferSlot = 0;
		static constexpr UINT VSPerCameraCBufferSlot = 1;

		inline static constexpr size_t maxPointsPerBatch = 1024;
	public:
		DebugPipelineDX11(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);


		void Bind(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);

		template<class Func>
		void Bind(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext, Func func)
		{
			Bind(deviceContext);
			func(MakeRenderInterface(deviceContext));
		}

		DebugRenderInterfaceDX11 MakeRenderInterface(TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext)
		{
			return { deviceContext, *this };
		}
	};

	template<std::invocable Func>
	void DebugRenderInterfaceDX11::CameraPass(const Camera& camera, std::optional<TypedD3D11::Wrapper<ID3D11RenderTargetView>> target, Func func)
	{
		if (target)
		{
			deviceContext->OMSetRenderTargets(target.value(), nullptr);
			D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(target.value()->GetResource())->GetDesc();
			D3D11_VIEWPORT viewports;
			viewports.TopLeftX = 0;
			viewports.TopLeftY = 0;
			viewports.MinDepth = 0;
			viewports.MaxDepth = 1;
			viewports.Width = static_cast<FLOAT>(desc.Width);
			viewports.Height = static_cast<FLOAT>(desc.Height);
			deviceContext->RSSetViewports(viewports);
		}

		UpdateConstantBuffer(deviceContext, m_debugRenderer.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
		});
		deviceContext->VSSetConstantBuffers(DebugPipelineDX11::VSPerCameraCBufferSlot, m_debugRenderer.cameraBuffer);
		func();
	}
}