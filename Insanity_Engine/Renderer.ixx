module;

#include <d3d11_4.h>
#include <utility>
#include <vector>
#include <unordered_map>
#include <span>
#include <numeric>
#include <numbers>
#include <algorithm>

export module InsanityEngine:Renderer;
import xk.Math;
import TypedD3D11;
import InsanityEngine.Container.StableVector;

using namespace xk::Math;

namespace InsanityEngine::Renderer
{
	export struct Camera
	{
		xk::Math::Matrix<float, 4, 4> viewPerspectiveTransform = Matrix<float, 4, 4>::Identity();

		Camera() = default;
		Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective);
	};

	struct SpritePipeline
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
		SpritePipeline();
	};

	export void DrawScene(TypedD3D::Wrapper<ID3D11RenderTargetView> target, const Camera& camera);

	export TypedD3D::Wrapper<ID3D11Device> GetDevice();
	export TypedD3D::Wrapper<ID3D11DeviceContext> GetDeviceContext();


	export class SpriteHandle
	{
		GenerationHandle handle;

	public:
		SpriteHandle() = default;
		SpriteHandle(std::nullptr_t) {}
		SpriteHandle(GenerationHandle handle) : handle{ handle } {}
		SpriteHandle(const SpriteHandle&) = default;
		SpriteHandle(SpriteHandle&& other) noexcept :
			handle{ std::exchange(other.handle, null_handle) }
		{
		}
		SpriteHandle& operator=(const SpriteHandle&) = default;
		SpriteHandle& operator=(std::nullptr_t)
		{
			handle = null_handle;
			return *this;
		}
		SpriteHandle& operator=(SpriteHandle&& other) noexcept
		{
			SpriteHandle temp{ std::move(other) };
			std::swap(*this, temp);
			return *this;
		}

	public:
		GenerationHandle Get() const { return handle; }

		void SetTransform(const Matrix<float, 4, 4>& transform);
		void SetTransform(Vector<float, 2> position, Degree<float> rotation, Vector<float, 2> scale = { 1, 1 });
		void SetTexture(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture);
		void SetPriority(int priority);

		TypedD3D11::Wrapper<ID3D11ShaderResourceView> GetTexture() const;
	};

	export class UniqueSpriteHandle
	{
		SpriteHandle handle;

	public:
		UniqueSpriteHandle() = default;
		UniqueSpriteHandle(std::nullptr_t) {}
		UniqueSpriteHandle(SpriteHandle handle) : handle{ handle } {}
		UniqueSpriteHandle(const UniqueSpriteHandle&) = delete;
		UniqueSpriteHandle(UniqueSpriteHandle&& other) noexcept :
			handle{ std::exchange(other.handle, null_handle) }
		{
		}
		UniqueSpriteHandle& operator=(const UniqueSpriteHandle&) = delete;
		UniqueSpriteHandle& operator=(std::nullptr_t)
		{
			UniqueSpriteHandle temp{ std::move(*this) };
			return *this;
		}
		UniqueSpriteHandle& operator=(UniqueSpriteHandle&& other) noexcept
		{
			UniqueSpriteHandle temp{ std::move(other) };
			std::swap(*this, temp);
			return *this;
		}

		~UniqueSpriteHandle();

	public:
		SpriteHandle Get() const { return handle; }

		void SetTransform(const Matrix<float, 4, 4>& transform) { handle.SetTransform(transform); }
		void SetTransform(Vector<float, 2> position, Degree<float> rotation, Vector<float, 2> scale = { 1, 1 }) { handle.SetTransform(position, rotation, scale); }
		void SetTexture(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture) { handle.SetTexture(texture); }
		void SetPriority(int priority) { handle.SetPriority(priority); }

		TypedD3D11::Wrapper<ID3D11ShaderResourceView> GetTexture() const { return handle.GetTexture(); }
	};

	export UniqueSpriteHandle NewSprite(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture);

	struct DebugPipeline
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
		DebugPipeline();
	};

	namespace Debug
	{
		Vector<float, 4> currentColor{ 1, 1, 1, 1 };
		std::unordered_map<Vector<float, 4>, std::vector<Vector<float, 3>>> batches;
		std::unordered_map<int, std::vector<Vector<float, 3>>> batchesTest;

		constexpr auto bias = xk::Math::Vector{ 0.f, 0.f, 1.f };
		auto& GetCurrentBatch()
		{
			if(auto it = batches.find(currentColor); it != batches.end())
				return it->second;
			return batches.insert({ currentColor, {} }).first->second;
		}

		export void DrawLine(xk::Math::Vector<float, 3> start, xk::Math::Vector<float, 3> end)
		{
			auto& batch = GetCurrentBatch();
			batch.push_back(start + bias);
			batch.push_back(end + bias);
		}

		//Pairs of points are expected
		export void DrawLines(std::span<xk::Math::Vector<float, 3>> points)
		{
			auto& batch = GetCurrentBatch();
			std::transform(points.begin(), points.end(), points.begin(), [](auto p) { return p + bias; });
			batch.insert(batch.end(), points.begin(), points.end());
		}

		export template<size_t Count>
			void DrawLines(std::array<xk::Math::Vector<float, 3>, Count> points)
		{
			DrawLine(std::span{ points });
		}

		export void DrawConnectedLines(std::span<xk::Math::Vector<float, 3>> points)
		{
			auto& batch = GetCurrentBatch();
			batch.reserve(batch.size() + points.size() * 2);
			std::transform(points.begin(), points.end(), points.begin(), [](auto p) { return p + bias; });

			for(size_t i = 0; i < points.size() - 1; i++)
			{
				batch.push_back(points[i]);
				batch.push_back(points[i + 1]);
			}
			batch.push_back(points.back());
			batch.push_back(points.front());
		}

		export template<size_t Count>
			void DrawConnectedLines(std::array<xk::Math::Vector<float, 3>, Count> points)
		{
			DrawConnectedLines(std::span{ points });
		}

		export void DrawSquare(xk::Math::Vector<float, 3> center, xk::Math::Vector<float, 3> halfSize)
		{
			const xk::Math::Vector<float, 3> bl = center - halfSize;
			const xk::Math::Vector<float, 3> tr = center + halfSize;
			const xk::Math::Vector<float, 3> tl{ bl.X(), tr.Y() };
			const xk::Math::Vector<float, 3> br{ tr.X(), bl.Y() };
			DrawConnectedLines(std::array{ bl, tl, tr, br });
		}

		export void DrawCircle(xk::Math::Vector<float, 3> center, float radius)
		{
			static constexpr size_t pointResolution = 64;
			std::array<xk::Math::Vector<float, 3>, pointResolution> points;

			float angleIncrements = static_cast<float>(std::numbers::pi_v<double> *2 / (pointResolution));
			for(size_t i = 0; i < points.size(); i++)
			{
				points[i] = center + xk::Math::Vector<float, 3>{ std::cos(angleIncrements* i), std::sin(angleIncrements* i), 0 } *radius;
				//points[(i + points.size() - 1) % points.size()] = points[i];
			}

			DrawConnectedLines(points);
		}

		export void SetColor(xk::Math::Vector<float, 4> rgba)
		{
			currentColor = rgba;
		}

		export void ClearBuffer()
		{
			batches.clear();
		}
	}
}