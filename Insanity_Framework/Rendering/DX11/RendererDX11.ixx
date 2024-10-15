module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3d11sdklayers.h>
#include <span>
#include <concepts>
#include <type_traits>
#include <filesystem>

export module InsanityFramework.RendererDX11;
export import TypedD3D11;
export import TypedDXGI;
export import xk.Math;
using namespace TypedD3D;

namespace InsanityFramework
{
	export TypedD3D11::Wrapper<ID3D11ShaderResourceView> CreateTexture(std::filesystem::path path, TypedD3D11::Wrapper<ID3D11Device> device);

	export class RendererDX11;

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

	export template<class Ty>
		concept RenderPipeline = requires (Ty pipeline, RendererDX11 & renderer)
	{
		pipeline.Bind(renderer);
		requires std::is_class_v<decltype(pipeline.MakeRenderInterface(renderer))>;
	};

	class RendererDX11
	{
	private:
		TypedD3D11::Wrapper<ID3D11Device> m_device;
		Microsoft::WRL::ComPtr<ID3D11Debug> m_debugDevice;
		TypedD3D11::Wrapper<ID3D11DeviceContext> m_deviceContext;
		TypedDXGI::Wrapper<IDXGISwapChain1> m_swapChain;
		TypedD3D11::Wrapper<ID3D11RenderTargetView> m_backBuffer;
		TypedD3D11::Wrapper<ID3D11DepthStencilView> m_backDepthBuffer;

	public:
		RendererDX11(HWND handle);
		~RendererDX11();

	public:
		template<RenderPipeline Ty, std::invocable<decltype(std::declval<Ty>().MakeRenderInterface(std::declval<RendererDX11&>()))> Func>
		void BindPipeline(Ty& pipeline, Func func)
		{
			pipeline.Bind(*this);
			func(pipeline.MakeRenderInterface(*this));
		}

		void DebugPrintMemory()
		{
			m_debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		}

		TypedD3D11::Wrapper<ID3D11Device> GetDevice() const { return m_device; }
		TypedD3D11::Wrapper<ID3D11DeviceContext> GetDeviceContext() const { return m_deviceContext; }
		TypedD3D11::Wrapper<IDXGISwapChain1> GetSwapChain() const { return m_swapChain; }
		TypedD3D11::Wrapper<ID3D11RenderTargetView> GetSwapChainBackBuffer() { return m_backBuffer; }
		TypedD3D11::Wrapper<ID3D11DepthStencilView> GetSwapChainBackDepthBuffer() { return m_backDepthBuffer; }
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
		RendererDX11& m_renderer;
		SpritePipelineDX11& m_spriteRenderer;

	public:
		SpriteRenderInterfaceDX11(RendererDX11& renderer, SpritePipelineDX11& spriteRenderer) :
			m_renderer{ renderer },
			m_spriteRenderer{ spriteRenderer }
		{
		}

		template<std::invocable<Camera> Func>
		void CameraPass(const Camera& camera, Func func);

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

		void Bind(RendererDX11& renderer);

		SpriteRenderInterfaceDX11 MakeRenderInterface(RendererDX11& renderer)
		{
			return { renderer, *this };
		}
	};

	template<std::invocable<Camera> Func>
	void SpriteRenderInterfaceDX11::CameraPass(const Camera& camera, Func func)
	{
		UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_spriteRenderer.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
		});
		m_renderer.GetDeviceContext()->VSSetConstantBuffers(SpritePipelineDX11::VSPerCameraCBufferSlot, m_spriteRenderer.cameraBuffer);
		func(camera);
	}

	export struct DebugPipelineDX11;

	export class DebugRenderInterfaceDX11
	{
	private:
		RendererDX11& m_renderer;
		DebugPipelineDX11& m_debugRenderer;

	public:
		DebugRenderInterfaceDX11(RendererDX11& renderer, DebugPipelineDX11& debugRenderer) :
			m_renderer{ renderer },
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

		template<std::invocable<Camera> Func>
		void CameraPass(const Camera& camera, Func func);
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

		void Bind(RendererDX11& renderer);

		DebugRenderInterfaceDX11 MakeRenderInterface(RendererDX11& renderer)
		{
			return { renderer, *this };
		}
	};

	template<std::invocable<Camera> Func>
	void DebugRenderInterfaceDX11::CameraPass(const Camera& camera, Func func)
	{
		UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_debugRenderer.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
		});
		m_renderer.GetDeviceContext()->VSSetConstantBuffers(DebugPipelineDX11::VSPerCameraCBufferSlot, m_debugRenderer.cameraBuffer);
		func(camera);
	}
}