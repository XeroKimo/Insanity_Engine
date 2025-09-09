module;

#include <d3d11_4.h>
#include <utility>

export module InsanityEngine.Renderer;
import xk.Math;
import TypedD3D11;
import InsanityEngine.Container.StableVector;

using namespace xk::Math;

namespace InsanityEngine::Renderer
{
	export struct Camera
	{
		xk::Math::Matrix<float, 4, 4> viewPerspectiveTransform;

		Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective);
	};



	export class Lifetime
	{
		friend Lifetime Initialize(bool enableDebug);

	private:
		bool engaged = false;

	private:
		Lifetime() = default;
		Lifetime(bool engaged) : engaged{ engaged } {}

	public:
		Lifetime(const Lifetime&) = delete;
		Lifetime(Lifetime&& other) noexcept :
			engaged{ std::exchange(other.engaged, false) }
		{

		}
		~Lifetime();
		Lifetime& operator=(const Lifetime&) = delete;
		Lifetime& operator=(Lifetime&& other) noexcept
		{
			Lifetime temp{ std::move(other) };
			std::swap(engaged, temp.engaged);
			return *this;
		}

	public:
		//Invoking this means you must manually call shutdown
		void Disengage() noexcept { engaged = false; }
	};

	export Lifetime Initialize(bool enableDebug);
	export void Shutdown();

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
}