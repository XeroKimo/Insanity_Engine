module;

#include <vector>
#include <cassert>
#include <span>
#include <functional>
#include <concepts>
#include <typeindex>
#include <unordered_map>
#include <memory>

export module InsanityFramework.ECS;
export import xk.Math;

namespace InsanityFramework
{
	export class Object;
	export class GameObject;

	export class Scene;
	export enum class VirtualConstructorPassthrough {};



	export template<bool isConst>
	class AnyRefT
	{
	private:
		const std::type_info* type;
		std::conditional_t<isConst, const void*, void*> ptr;

	public:
		template<class Ty>
			requires (!std::same_as<AnyRefT, Ty>)
		AnyRefT(Ty& obj) :
			type{ &typeid(Ty) },
			ptr{ &obj }
		{

		}

		AnyRefT(const AnyRefT& other) :
			type{ other.type },
			ptr{ other.ptr }
		{

		}

		AnyRefT(AnyRefT&& other) noexcept :
			type{ std::exchange(other.type, nullptr) },
			ptr{ std::exchange(other.ptr, nullptr) }
		{

		}

		template<class Ty>
		Ty& operator=(const Ty& obj)
		{
			auto& self = As<Ty>();
			self = obj;
			return self;
		}

		template<class Ty>
		Ty& operator=(Ty&& obj)
		{
			auto& self = As<Ty>();
			self = std::move(obj);
			return self;
		}

		template<class Ty>
		Ty& As()
		{
			if(*type != typeid(Ty))
				throw std::exception{};

			return *static_cast<Ty*>(ptr);
		}

		template<class Ty>
		const Ty& As() const
		{
			if(*type != typeid(Ty))
				throw std::exception{};

			return *static_cast<const Ty*>(ptr);
		}
	};

	export using AnyRef = AnyRefT<false>;
	export using AnyConstRef = AnyRefT<true>;

	export class SystemsViewer
	{

	public:
		virtual AnyRef GetSystem(const std::type_info& type) = 0;
		virtual AnyConstRef GetSystem(const std::type_info& type) const = 0;

		template<class Ty>
		Ty& GetSystem()
		{
			return GetSystem(typeid(Ty)).As<Ty>();
		}

		template<class Ty>
		const Ty& GetSystem() const
		{
			return GetSystem(typeid(Ty)).As<Ty>();
		}
	};

	export struct Transform
	{
		xk::Math::Vector<float, 3> position;
		xk::Math::Degree<float> rotation;
		xk::Math::Vector<float, 3> scale{ xk::Math::Uniform{ 1 } };

		friend Transform operator+(Transform lh, const Transform& rh)
		{
			lh.position += rh.position;
			lh.rotation += rh.rotation;
			lh.scale = xk::Math::HadamardProduct(lh.scale, rh.scale);
			return lh;
		}

		friend Transform operator-(Transform lh, const Transform& rh)
		{
			lh.position -= rh.position;
			lh.rotation -= rh.rotation;
			lh.scale = xk::Math::HadamardSafeDivision(lh.scale, rh.scale);
			return lh;
		}

		friend bool operator==(const Transform& lh, const Transform& rh)
		{
			return lh.position == rh.position && lh.rotation == rh.rotation && lh.scale == rh.scale;
		}
	};

	export class TransformNode;

	export struct LocalTransformType
	{
		Transform value;
	};

	export struct WorldTransformType
	{
		Transform value;
	};

	export struct LocalTransformInitializer
	{
		TransformNode* parent = nullptr;
		LocalTransformType transform;
	};

	export struct WorldTransformInitializer
	{
		TransformNode* parent = nullptr;
		WorldTransformType transform;
	};

	enum class TransformDestructorLogic
	{
		Null_Parent_Keep_Local_Transform,
		Null_Parent_Keep_World_Transform,
		Reparent_Keep_Local_Transform,
		Reparent_Keep_World_Transform,
	};

	template<class Func>
	struct ReturnType;

	template<class Ret, class... Params>
	struct ReturnType<Ret(*)(Params...)>
	{
		using type = Ret;
	};

	template<class Ty, class Ret, class... Params>
	struct ReturnType<Ret(Ty::*)(Params...)>
	{
		using type = Ret;
	};

	template<class Ty, class Ret, class... Params>
	struct ReturnType<Ret(Ty::*)(Params...) const>
	{
		using type = Ret;
	};

	
	template<auto Setter, auto Getter>
	class TransformProxyType;

	template<auto Setter, auto Getter, size_t Index>
	class VectorProxyType
	{
		using proxy_type = VectorProxyType<Setter, Getter, Index>;
		//inline static const auto Setter = &TransformNode::SetLocalPosition;
		//inline static const auto Getter = &TransformNode::GetLocalPosition;
		using value_type = typename ReturnType<decltype(Getter)>::type::value_type;

	private:
		TransformNode* node;

	public:
		void Set(value_type value)
		{
			auto temp = std::invoke(Getter, node);
			temp[Index] = value;
			std::invoke(Setter, node, temp);
		}
		value_type Get() const { return std::invoke(Getter, node)[Index]; }

	public:
		VectorProxyType(TransformNode* node) :
			node{ node }
		{

		}

		value_type operator=(const value_type& other)
		{
			Set(other);
			return Get();
		}

		friend bool operator==(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() == rh;
		}

		friend bool operator==(const value_type& lh, const proxy_type& rh)
		{
			return lh == rh.Get();
		}

		value_type operator-() const
		{
			return -Get();
		}

		value_type operator+=(const value_type& other)
		{
			Set(Get() + other);
			return Get();
		}

		value_type operator-=(const value_type& other)
		{
			Set(Get() - other);
			return Get();
		}

		value_type operator*=(const value_type& other)
		{
			Set(Get() * other);
			return Get();
		}

		value_type operator/=(const value_type& other)
		{
			Set(Get() * other);
			return Get();
		}

		friend value_type operator+(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() + rh;
		}

		friend value_type operator+(const value_type& lh, const proxy_type& rh)
		{
			return lh + rh.Get();
		}

		friend value_type operator-(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() - rh;
		}

		friend value_type operator-(const value_type& lh, const proxy_type& rh)
		{
			return lh - rh.Get();
		}

		friend value_type operator*(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() * rh;
		}

		friend value_type operator*(const value_type& lh, const proxy_type& rh)
		{
			return lh * rh.Get();
		}

		friend value_type operator/(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() / rh;
		}

		friend value_type operator/(const value_type& lh, const proxy_type& rh)
		{
			return lh / rh.Get();
		}

		value_type operator=(const proxy_type& other)
		{
			return operator=(other.Get());
		}

		friend bool operator==(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() == rh.Get();
		}

		value_type operator+=(const proxy_type& other)
		{
			return operator+=(other.Get());
		}

		value_type operator-=(const proxy_type& other)
		{
			return operator-=(other.Get());
		}

		value_type operator*=(const proxy_type& other)
		{
			return operator*=(other.Get());
		}

		value_type operator/=(const proxy_type& other)
		{
			return operator/=(other.Get());
		}

		friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() + rh;
		}

		friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() - rh;
		}

		friend value_type operator*(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() * rh;
		}

		friend value_type operator/(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() / rh;
		}

		operator value_type() const { return Get(); }
	};

	template<auto Setter, auto Getter>
		requires std::same_as<typename ReturnType<decltype(Getter)>::type, xk::Math::Vector<float, 3>>
	class TransformProxyType<Setter, Getter>
	{
		using proxy_type = TransformProxyType<Setter, Getter>;
		using value_type = typename ReturnType<decltype(Getter)>::type;
		using underlying_type = value_type::value_type;

	private:
		TransformNode* node;

	public:
		void Set(value_type value) { std::invoke(Setter, node, value); }
		value_type Get() const { return std::invoke(Getter, node); }

	public:
		TransformProxyType(TransformNode* node) :
			node{ node }
		{

		}

		value_type operator=(const value_type& other)
		{
			Set(other);
			return Get();
		}

		friend bool operator==(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() == rh;
		}

		friend bool operator==(const value_type& lh, const proxy_type& rh)
		{
			return lh == rh.Get();
		}

		value_type operator+=(const value_type& other)
		{
			Set(Get() + other);
			return Get();
		}

		value_type operator-=(const value_type& other)
		{
			Set(Get() - other);
			return Get();
		}

		value_type operator-() const
		{
			return -Get();
		}

		//value_type operator*=(const value_type& other)
		//{
		//	Set(Get() * other);
		//	return Get();
		//}

		//value_type operator/=(const value_type& other)
		//{
		//	Set(Get() * other);
		//	return Get();
		//}

		friend value_type operator+(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() + rh;
		}

		friend value_type operator+(const value_type& lh, const proxy_type& rh)
		{
			return lh + rh.Get();
		}

		friend value_type operator-(const proxy_type& lh, const value_type& rh)
		{
			return lh.Get() - rh;
		}

		friend value_type operator-(const value_type& lh, const proxy_type& rh)
		{
			return lh - rh.Get();
		}

		//friend value_type operator*(const proxy_type& lh, const value_type& rh)
		//{
		//	return lh.Get() * rh;
		//}

		//friend value_type operator*(const value_type& lh, const proxy_type& rh)
		//{
		//	return lh * rh.Get();
		//}

		//friend value_type operator/(const proxy_type& lh, const value_type& rh)
		//{
		//	return lh.Get() / rh;
		//}

		//friend value_type operator/(const value_type& lh, const proxy_type& rh)
		//{
		//	return lh / rh.Get();
		//}

		value_type operator=(const proxy_type& other)
		{
			return operator=(other.Get());
		}

		friend bool operator==(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() == rh.Get();
		}

		value_type operator+=(const proxy_type& other)
		{
			return operator+=(other.Get());
		}

		value_type operator-=(const proxy_type& other)
		{
			return operator-=(other.Get());
		}

		//value_type operator*=(const proxy_type& other)
		//{
		//	return operator*=(other.Get());
		//}

		//value_type operator/=(const proxy_type& other)
		//{
		//	return operator/=(other.Get());
		//}

		friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() + rh;
		}

		friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
		{
			return lh.Get() - rh;
		}

		//friend value_type operator*(const proxy_type& lh, const proxy_type& rh)
		//{
		//	return lh.Get() * rh;
		//}

		//friend value_type operator/(const proxy_type& lh, const proxy_type& rh)
		//{
		//	return lh.Get() / rh;
		//}

		value_type operator*=(const underlying_type& other)
		{
			Set(Get() * other);
			return Get();
		}

		value_type operator/=(const underlying_type& other)
		{
			Set(Get() * other);
			return Get();
		}

		friend value_type operator*(const proxy_type& lh, const underlying_type& rh)
		{
			return lh.Get() * rh;
		}

		friend value_type operator*(const underlying_type& lh, const proxy_type& rh)
		{
			return lh * rh.Get();
		}

		friend value_type operator/(const proxy_type& lh, const underlying_type& rh)
		{
			return lh.Get() / rh;
		}

		//friend value_type operator/(const underlying_type& lh, const proxy_type& rh)
		//{
		//	return lh / rh.Get();
		//}

		VectorProxyType<Setter, Getter, 0> X() { return { node }; };
		VectorProxyType<Setter, Getter, 1> Y() { return { node }; };
		VectorProxyType<Setter, Getter, 2> Z() { return { node }; };

		template<size_t... Index>
		auto Swizzle() { return Get().Swizzle<Index...>(); }

		operator value_type() const { return Get(); }
	};

	export class TransformNode
	{
	private:
		class LocalTransformProxy;
		class WorldTransformProxy;

	private:
		TransformNode* parent = nullptr;
		std::vector<TransformNode*> children;
		Transform local;
		mutable Transform worldCache;
		mutable bool worldCacheDirty = false;

	public:
		TransformDestructorLogic destructionLogic = TransformDestructorLogic::Reparent_Keep_Local_Transform;

	public:
		TransformNode() = default;

		TransformNode(TransformNode* parent)
		{
			SetParent(parent);
		}

		TransformNode(LocalTransformInitializer initializer)
		{
			SetParent(initializer.parent);
			SetLocalTransform(initializer.transform.value);
		}

		TransformNode(WorldTransformInitializer initializer)
		{
			SetParent(initializer.parent);
			SetWorldTransform(initializer.transform.value);
		}

		TransformNode(const TransformNode&) = delete;
		TransformNode(TransformNode&& other) noexcept
		{
			SetParent(other.parent);
			for(TransformNode* child : other.children)
			{
				child->SetParent(this);
			}
			other.SetParent(nullptr);
		}

		TransformNode& operator=(const TransformNode&) = delete;
		TransformNode& operator=(TransformNode&& other) noexcept
		{
			SetParent(other.parent);
			for(TransformNode* child : other.children)
			{
				child->SetParent(this);
			}
			other.SetParent(nullptr);

			return *this;
		}

		~TransformNode()
		{
			switch(destructionLogic)
			{
			case TransformDestructorLogic::Null_Parent_Keep_Local_Transform:
				while(!children.empty())
				{
					children.back()->SetParent(nullptr);
				}
				break;
			case TransformDestructorLogic::Null_Parent_Keep_World_Transform:
				while(!children.empty())
				{
					children.back()->SetParentKeepWorldTransform(nullptr);
				}
				break;
			case TransformDestructorLogic::Reparent_Keep_Local_Transform:
				while(!children.empty())
				{
					children.back()->SetParent(parent);
				}
				break;
			case TransformDestructorLogic::Reparent_Keep_World_Transform:
				while(!children.empty())
				{
					children.back()->SetParentKeepWorldTransform(parent);
				}
				break;
			default:
				assert((false && "Case unhandled"));
				break;
			}

			SetParent(nullptr);
		}

		void SetParent(TransformNode* newParent)
		{
			if(parent == newParent)
				return;

			DetectCyclicParent(newParent);

			TransformNode* oldParent = parent;

			if(newParent)
				newParent->children.push_back(this);

			parent = newParent;
			SetWorldCacheDirty();

			if(oldParent)
				std::erase(oldParent->children, this);
		}

		void SetParentKeepWorldTransform(TransformNode* newParent)
		{
			auto oldWorldTransform = GetWorldTransform();
			SetParent(newParent);
			SetWorldTransform(oldWorldTransform);
		}

		TransformNode* GetParent() const noexcept
		{
			return parent;
		}

		std::span<TransformNode* const> GetChildren() const noexcept
		{
			return { children };
		}

		LocalTransformProxy LocalTransform()
		{
			return { this };
		}

		WorldTransformProxy WorldTransform()
		{
			return { this };
		}

	private:
		void DetectCyclicParent(TransformNode* newParent)
		{
			for(auto parent = newParent; parent; parent = parent->GetParent())
			{
				if(parent == this)
					throw std::exception{};
			}
		}

		void SetLocalPosition(xk::Math::Vector<float, 3> position)
		{
			local.position = position;
			SetWorldCacheDirty();
		}

		void SetLocalRotation(xk::Math::Degree<float> rotation)
		{
			local.rotation = rotation;
			SetWorldCacheDirty();
		}

		void SetLocalScale(xk::Math::Vector<float, 3> scale)
		{
			local.scale = scale;
			SetWorldCacheDirty();
		}

		void SetLocalTransform(Transform transform)
		{
			local = transform;
			SetWorldCacheDirty();
		}

		xk::Math::Vector<float, 3> GetLocalPosition() const
		{
			return local.position;
		}

		xk::Math::Degree<float> GetLocalRotation() const
		{
			return local.rotation;
		}

		xk::Math::Vector<float, 3> GetLocalScale() const
		{
			return local.scale;
		}

		Transform GetLocalTransform() const
		{
			RecalculateWorldTransform();
			return local;
		}

		void SetWorldPosition(xk::Math::Vector<float, 3> position)
		{
			if(parent)
			{
				SetLocalPosition(position - parent->GetWorldPosition());
			}
			else
			{
				SetLocalPosition(position);
			}
		}

		void SetWorldRotation(xk::Math::Degree<float> rotation)
		{
			if(parent)
			{
				SetLocalRotation(rotation - parent->GetWorldRotation());
			}
			else
			{
				SetLocalRotation(rotation);
			}
		}

		void SetWorldScale(xk::Math::Vector<float, 3> scale)
		{
			if(parent)
			{
				SetLocalScale(xk::Math::HadamardSafeDivision(scale, parent->GetWorldScale()));
			}
			else
			{
				SetLocalScale(scale);
			}
		}

		void SetWorldTransform(Transform transform)
		{
			if(parent)
			{
				SetLocalTransform(transform - parent->GetWorldTransform());
			}
			else
			{
				SetLocalTransform(transform);
			}
		}

		xk::Math::Vector<float, 3> GetWorldPosition() const
		{
			RecalculateWorldTransform();
			return worldCache.position;
		}

		xk::Math::Degree<float> GetWorldRotation() const
		{
			RecalculateWorldTransform();
			return worldCache.rotation;
		}

		xk::Math::Vector<float, 3> GetWorldScale() const
		{
			RecalculateWorldTransform();
			return worldCache.scale;
		}

		Transform GetWorldTransform() const
		{
			RecalculateWorldTransform();
			return worldCache;
		}

	private:
		void SetWorldCacheDirty() noexcept
		{
			if(worldCacheDirty)
				return;

			worldCacheDirty = true;

			for(TransformNode* node : children)
			{
				node->SetWorldCacheDirty();
			}
		}

		void RecalculateWorldTransform() const
		{
			if(!worldCacheDirty)
				return;

			if(parent)
			{
				worldCache = parent->GetWorldTransform() + local;
			}
			else
			{
				worldCache = local;
			}

			worldCacheDirty = false;
		}

		class LocalTransformProxy
		{
			using PositionProxy = TransformProxyType<&TransformNode::SetLocalPosition, &TransformNode::GetLocalPosition>;
			class RotationProxy
			{
				using proxy_type = RotationProxy;
				inline static const auto Setter = &TransformNode::SetLocalRotation;
				inline static const auto Getter = &TransformNode::GetLocalRotation;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				RotationProxy(TransformNode* node) :
					node{ node }
				{

				}

				value_type operator=(const value_type& other)
				{
					Set(other);
					return Get();
				}

				friend bool operator==(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() == rh;
				}

				friend bool operator==(const value_type& lh, const proxy_type& rh)
				{
					return lh == rh.Get();
				}

				value_type operator+=(const value_type& other)
				{
					Set(Get() + other);
					return Get();
				}

				value_type operator-=(const value_type& other)
				{
					Set(Get() - other);
					return Get();
				}

				//value_type operator*=(const value_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//value_type operator/=(const value_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				friend value_type operator+(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() + rh;
				}

				friend value_type operator+(const value_type& lh, const proxy_type& rh)
				{
					return lh + rh.Get();
				}

				friend value_type operator-(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() - rh;
				}

				friend value_type operator-(const value_type& lh, const proxy_type& rh)
				{
					return lh - rh.Get();
				}

				//friend value_type operator*(const proxy_type& lh, const value_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator*(const value_type& lh, const proxy_type& rh)
				//{
				//	return lh * rh.Get();
				//}

				//friend value_type operator/(const proxy_type& lh, const value_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//friend value_type operator/(const value_type& lh, const proxy_type& rh)
				//{
				//	return lh / rh.Get();
				//}

				value_type operator=(const proxy_type& other)
				{
					return operator=(other.Get());
				}

				friend bool operator==(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() == rh.Get();
				}

				value_type operator+=(const proxy_type& other)
				{
					return operator+=(other.Get());
				}

				value_type operator-=(const proxy_type& other)
				{
					return operator-=(other.Get());
				}

				//value_type operator*=(const proxy_type& other)
				//{
				//	return operator*=(other.Get());
				//}

				//value_type operator/=(const proxy_type& other)
				//{
				//	return operator/=(other.Get());
				//}

				friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() + rh;
				}

				friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() - rh;
				}

				//friend value_type operator*(const proxy_type& lh, const proxy_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator/(const proxy_type& lh, const proxy_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//value_type operator*=(const underlying_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//value_type operator/=(const underlying_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//friend value_type operator*(const proxy_type& lh, const underlying_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator*(const underlying_type& lh, const proxy_type& rh)
				//{
				//	return lh * rh.Get();
				//}

				//friend value_type operator/(const proxy_type& lh, const underlying_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//friend value_type operator/(const underlying_type& lh, const proxy_type& rh)
				//{
				//	return lh / rh.Get();
				//}

				operator value_type() const { return Get(); }
			};
			using ScaleProxy = TransformProxyType<&TransformNode::SetLocalScale, &TransformNode::GetLocalScale>;


			using proxy_type = LocalTransformProxy;
			inline static const auto Setter = &TransformNode::SetLocalTransform;
			inline static const auto Getter = &TransformNode::GetLocalTransform;
			using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));

		private:
			TransformNode* node;

		public:
			void Set(value_type value) { std::invoke(Setter, node, value); }
			value_type Get() const { return std::invoke(Getter, node); }

			PositionProxy Position() { return PositionProxy{ node }; }
			RotationProxy Rotation() { return RotationProxy{ node }; }
			ScaleProxy Scale() { return ScaleProxy{ node }; }

		public:
			LocalTransformProxy(TransformNode* node) :
				node{ node }
			{

			}

			value_type operator=(const value_type& other)
			{
				Set(other);
				return Get();
			}

			friend bool operator==(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() == rh;
			}

			friend bool operator==(const value_type& lh, const proxy_type& rh)
			{
				return lh == rh.Get();
			}

			value_type operator+=(const value_type& other)
			{
				Set(Get() + other);
				return Get();
			}

			value_type operator-=(const value_type& other)
			{
				Set(Get() - other);
				return Get();
			}

			friend value_type operator+(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() + rh;
			}

			friend value_type operator+(const value_type& lh, const proxy_type& rh)
			{
				return lh + rh.Get();
			}

			friend value_type operator-(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() - rh;
			}

			friend value_type operator-(const value_type& lh, const proxy_type& rh)
			{
				return lh - rh.Get();
			}

			value_type operator=(const proxy_type& other)
			{
				return operator=(other.Get());
			}

			friend bool operator==(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() == rh.Get();
			}

			value_type operator+=(const proxy_type& other)
			{
				return operator+=(other.Get());
			}

			value_type operator-=(const proxy_type& other)
			{
				return operator-=(other.Get());
			}

			friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() + rh;
			}

			friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() - rh;
			}

			operator value_type() const { return Get(); }
		};

		class WorldTransformProxy
		{
			using PositionProxy = TransformProxyType<&TransformNode::SetWorldPosition, &TransformNode::GetWorldPosition>;
			class RotationProxy
			{
				using proxy_type = RotationProxy;
				inline static const auto Setter = &TransformNode::SetWorldRotation;
				inline static const auto Getter = &TransformNode::GetWorldRotation;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				RotationProxy(TransformNode* node) :
					node{ node }
				{

				}

				value_type operator=(const value_type& other)
				{
					Set(other);
					return Get();
				}

				friend bool operator==(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() == rh;
				}

				friend bool operator==(const value_type& lh, const proxy_type& rh)
				{
					return lh == rh.Get();
				}

				value_type operator+=(const value_type& other)
				{
					Set(Get() + other);
					return Get();
				}

				value_type operator-=(const value_type& other)
				{
					Set(Get() - other);
					return Get();
				}

				//value_type operator*=(const value_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//value_type operator/=(const value_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				friend value_type operator+(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() + rh;
				}

				friend value_type operator+(const value_type& lh, const proxy_type& rh)
				{
					return lh + rh.Get();
				}

				friend value_type operator-(const proxy_type& lh, const value_type& rh)
				{
					return lh.Get() - rh;
				}

				friend value_type operator-(const value_type& lh, const proxy_type& rh)
				{
					return lh - rh.Get();
				}

				//friend value_type operator*(const proxy_type& lh, const value_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator*(const value_type& lh, const proxy_type& rh)
				//{
				//	return lh * rh.Get();
				//}

				//friend value_type operator/(const proxy_type& lh, const value_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//friend value_type operator/(const value_type& lh, const proxy_type& rh)
				//{
				//	return lh / rh.Get();
				//}

				value_type operator=(const proxy_type& other)
				{
					return operator=(other.Get());
				}

				friend bool operator==(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() == rh.Get();
				}

				value_type operator+=(const proxy_type& other)
				{
					return operator+=(other.Get());
				}

				value_type operator-=(const proxy_type& other)
				{
					return operator-=(other.Get());
				}

				//value_type operator*=(const proxy_type& other)
				//{
				//	return operator*=(other.Get());
				//}

				//value_type operator/=(const proxy_type& other)
				//{
				//	return operator/=(other.Get());
				//}

				friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() + rh;
				}

				friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
				{
					return lh.Get() - rh;
				}

				//friend value_type operator*(const proxy_type& lh, const proxy_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator/(const proxy_type& lh, const proxy_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//value_type operator*=(const underlying_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//value_type operator/=(const underlying_type& other)
				//{
				//	Set(Get() * other);
				//	return Get();
				//}

				//friend value_type operator*(const proxy_type& lh, const underlying_type& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator*(const underlying_type& lh, const proxy_type& rh)
				//{
				//	return lh * rh.Get();
				//}

				//friend value_type operator/(const proxy_type& lh, const underlying_type& rh)
				//{
				//	return lh.Get() / rh;
				//}

				//friend value_type operator/(const underlying_type& lh, const proxy_type& rh)
				//{
				//	return lh / rh.Get();
				//}

				operator value_type() const { return Get(); }
			};
			using ScaleProxy = TransformProxyType<&TransformNode::SetWorldScale, &TransformNode::GetWorldScale>;

			using proxy_type = WorldTransformProxy;
			inline static const auto Setter = &TransformNode::SetWorldTransform;
			inline static const auto Getter = &TransformNode::GetWorldTransform;
			using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));

		private:
			TransformNode* node;

		public:
			void Set(value_type value) { std::invoke(Setter, node, value); }
			value_type Get() const { return std::invoke(Getter, node); }

			PositionProxy Position() { return PositionProxy{ node }; }
			RotationProxy Rotation() { return RotationProxy{ node }; }
			ScaleProxy Scale() { return ScaleProxy{ node }; }

		public:
			WorldTransformProxy(TransformNode* node) :
				node{ node }
			{

			}

			value_type operator=(const value_type& other)
			{
				Set(other);
				return Get();
			}

			friend bool operator==(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() == rh;
			}

			friend bool operator==(const value_type& lh, const proxy_type& rh)
			{
				return lh == rh.Get();
			}

			value_type operator+=(const value_type& other)
			{
				Set(Get() + other);
				return Get();
			}

			value_type operator-=(const value_type& other)
			{
				Set(Get() - other);
				return Get();
			}

			friend value_type operator+(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() + rh;
			}

			friend value_type operator+(const value_type& lh, const proxy_type& rh)
			{
				return lh + rh.Get();
			}

			friend value_type operator-(const proxy_type& lh, const value_type& rh)
			{
				return lh.Get() - rh;
			}

			friend value_type operator-(const value_type& lh, const proxy_type& rh)
			{
				return lh - rh.Get();
			}

			value_type operator=(const proxy_type& other)
			{
				return operator=(other.Get());
			}

			friend bool operator==(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() == rh.Get();
			}

			value_type operator+=(const proxy_type& other)
			{
				return operator+=(other.Get());
			}

			value_type operator-=(const proxy_type& other)
			{
				return operator-=(other.Get());
			}

			friend value_type operator+(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() + rh;
			}

			friend value_type operator-(const proxy_type& lh, const proxy_type& rh)
			{
				return lh.Get() - rh;
			}

			operator value_type() const { return Get(); }
		};
	};



	template<std::derived_from<InsanityFramework::Object> Ty>
	struct ObjectDeleter
	{
		void operator()(Ty* ptr);
	};

	export template<std::derived_from<InsanityFramework::Object> Ty>
	class UniqueObject
	{
	private:
		std::unique_ptr<Ty, ObjectDeleter<Ty>> ptr;

	public:
		UniqueObject() = default;
		UniqueObject(std::nullptr_t) : ptr{ nullptr } {}
		UniqueObject(Ty* ptr) : ptr{ ptr }
		{
			if(ptr)
				ptr->SetIsRootObject(false);
		}
		UniqueObject(const UniqueObject&) = delete;
		UniqueObject(UniqueObject&& other) noexcept :
			ptr{ std::move(other).ptr }
		{

		}

		UniqueObject& operator=(std::nullptr_t) { ptr = nullptr; return *this; }
		UniqueObject& operator=(const UniqueObject&) = delete;
		UniqueObject& operator=(UniqueObject&& other) noexcept
		{
			UniqueObject temp{ std::move(other) };
			swap(temp);
			return *this;
		}

		~UniqueObject() = default;

	public:
		auto release()
		{
			ptr->SetIsRootObject(true);
			return ptr.release();
		}

		void reset(Ty* newPtr) noexcept
		{
			ptr.reset(newPtr);
			if(newPtr)
				newPtr->SetIsRootObject(false);
		}

		void swap(UniqueObject& other) noexcept
		{
			ptr.swap(other.ptr);
		}

		auto get() { return ptr.get(); }
		auto get() const { return ptr.get(); }

		auto operator->() { return ptr.operator->(); }
		auto operator->() const { return ptr.operator->(); }

		decltype(auto) operator*() { return (ptr.operator*()); }
		decltype(auto) operator*() const { return (ptr.operator*()); }

		operator bool() const noexcept { return static_cast<bool>(ptr); }
	};


	export class ObjectManager
	{
	private:
		std::uint32_t lifetimesLockCounter = 0;

		class LockLifetime
		{
			std::uint32_t& counter;

		public:
			LockLifetime(std::uint32_t& counter) : counter{ counter }
			{
				counter += 1;
			}

			~LockLifetime()
			{
				counter -= 1;
			}
		};
		std::unordered_map<std::type_index, std::vector<Object*>> objects;
		std::vector<Object*> queuedConstruction;
		std::vector<Object*> queuedDestruction;
		SystemsViewer* systems;

	public:
		ObjectManager(SystemsViewer& systems) :
			systems{ &systems }
		{

		}
		ObjectManager(const ObjectManager&) = delete;
		ObjectManager(ObjectManager&& other) noexcept :
			objects{ std::move(other).objects },
			queuedConstruction{ std::move(other).queuedConstruction },
			queuedDestruction{ std::move(other).queuedDestruction }
		{

		}

		ObjectManager& operator=(const ObjectManager&) = delete;
		ObjectManager& operator=(ObjectManager&& other) noexcept
		{
			ObjectManager temp{ std::move(other) };
			objects.swap(temp.objects);
			queuedConstruction.swap(temp.queuedConstruction);
			queuedDestruction.swap(temp.queuedDestruction);

			return *this;
		}

		~ObjectManager();

	public:
		template<std::derived_from<Object> Ty, class... ConstructorArgs>
		UniqueObject<Ty> NewObject(ConstructorArgs&&... args)
		{
			UniqueObject<Ty> object{ new Ty(*this, std::forward<ConstructorArgs>(args)...) };
			if(lifetimesLockCounter == 0)
			{
				objects[typeid(Ty)].push_back(object.get());
			}
			else
			{
				queuedConstruction.push_back(object.get());
			}

			return object;
		}

		void DeleteObject(Object* ptr);

		template<std::invocable Func>
		void LockLifetimes(Func func)
		{
			auto _ = LockLifetime{ lifetimesLockCounter };
			func();
		}

		template<std::derived_from<Object> Ty, std::invocable<Ty&> Func>
		void ForEachExactType(Func func)
		{
			auto _ = LockLifetime{ lifetimesLockCounter };
			for(Object* object : objects.at(typeid(Ty)))
			{
				func(*static_cast<Ty*>(object));
			}
		}

		template<std::derived_from<Object> Ty, std::invocable<Ty&> Func>
		void ForEach(Func func)
		{
			auto _ = LockLifetime{ lifetimesLockCounter };
			for(auto& [type, objectVec] : objects)
			{
				if(objectVec.size() == 0)
					continue;

				if(!dynamic_cast<Ty*>(objectVec.front()))
					continue;

				for(Object* object : objectVec)
				{
					func(*static_cast<Ty*>(object));
				}
			}
		}

		template<class Ty, std::invocable<Ty&> Func>
		void ForEachInterface(Func func)
		{
			auto _ = LockLifetime{ lifetimesLockCounter };
			for(auto& [type, objectVec] : objects)
			{
				if(objectVec.size() == 0)
					continue;

				Ty* temp = dynamic_cast<Ty*>(objectVec.front());

				if(!temp)
					continue;

				auto offset = reinterpret_cast<char*>(temp) - reinterpret_cast<char*>(objectVec.front());
				assert(reinterpret_cast<Ty*>(reinterpret_cast<char*>(objectVec.front()) + offset) == temp);

				for(Object* object : objectVec)
				{
					func(*reinterpret_cast<Ty*>(reinterpret_cast<char*>(object) + offset));
				}
			}
		}

		void FlushLifetimes();

		template<class Ty>
		Ty& GetSystem() { return systems->GetSystem<Ty>(); }

		template<class Ty>
		const Ty& GetSystem() const { return systems->GetSystem<Ty>(); }
	};

	template<std::derived_from<InsanityFramework::Object> Ty>
	void ObjectDeleter<Ty>::operator()(Ty* ptr)
	{
		ptr->DeleteObject(ptr);
	}

	export class ObjectInterface
	{
	private:
		ObjectManager* manager;

	protected:
		ObjectInterface(VirtualConstructorPassthrough)
		{

		}

		ObjectInterface(ObjectManager& manager) :
			manager{ &manager }
		{

		}

		~ObjectInterface() = default;

	protected:
		template<std::derived_from<Object> Ty, class... ConstructorArgs>
		UniqueObject<Ty> NewObject(ConstructorArgs&&... args)
		{
			return manager->NewObject<Ty>(std::forward<ConstructorArgs>(args)...);
		}

		//Only use this if you know what you're doing
		//Prefer to rely on UniqueObject or SharedObject to manage the lifetimes of the created objects
		void DeleteObject(Object* object)
		{
			manager->DeleteObject(object);
		}

		template<class Ty>
		Ty& GetSystem() { return manager->GetSystem<Ty>(); }

		template<class Ty>
		const Ty& GetSystem() const { return manager->GetSystem<Ty>(); }
	};

	export class Object : public virtual ObjectInterface
	{
		template<std::derived_from<InsanityFramework::Object> Ty>
		friend class UniqueObject;

		template<std::derived_from<InsanityFramework::Object> Ty>
		friend struct ObjectDeleter;

		friend class ObjectManager;

	private:
		bool isRoot = false;

	protected:
		Object() : ObjectInterface{ VirtualConstructorPassthrough{} } {}

	public:
		Object(ObjectManager& manager) :
			ObjectInterface{ manager }
		{

		}

		virtual ~Object() = default;

	private:
		void SetIsRootObject(bool root)
		{
			isRoot = root;
		}
	};


	void ObjectManager::DeleteObject(Object* ptr)
	{
		if(lifetimesLockCounter == 0)
		{
			std::erase(objects[typeid(*ptr)], ptr);
			delete ptr;
		}
		else
		{
			queuedDestruction.push_back(ptr);
		}
	}

	void ObjectManager::FlushLifetimes()
	{
		assert(lifetimesLockCounter == 0);

		for(Object* object : queuedDestruction)
		{
			std::erase(objects[typeid(*object)], object);
			delete object;
		}
		queuedDestruction.clear();

		for(Object* object : queuedConstruction)
		{
			objects[typeid(*object)].push_back(object);
		}
		queuedConstruction.clear();
	}

	export class SceneAware
	{
		Scene* scene;

	public:
		SceneAware(VirtualConstructorPassthrough) {}
		SceneAware(Scene& scene) :
			scene{ &scene }
		{

		}

		Scene& GetScene() noexcept { return *scene; }
		const Scene& GetScene() const noexcept { return *scene; }
	};

	export class GameObject : public Object, public virtual TransformNode
	{
	protected:
		GameObject() :
			ObjectInterface{ VirtualConstructorPassthrough{} }
		{

		}

	public:
		GameObject(ObjectManager& manager) :
			ObjectInterface{ manager }
		{

		}

		virtual ~GameObject() = default;
	};

	ObjectManager::~ObjectManager()
	{
		assert(lifetimesLockCounter == 0);
		for(auto& [type, objects] : objects)
		{
			for(Object* object : objects)
			{
				if(object->isRoot)
					delete object;
			}
		}
	}



	export class SceneSystem : public SceneAware
	{

	public:
		using SceneAware::SceneAware;

		virtual ~SceneSystem() = default;
	};

	export class Scene : public SystemsViewer
	{
	private:
		std::unordered_map<std::type_index, AnyRef> externalSystems;
		std::unordered_map<std::type_index, std::unique_ptr<SceneSystem>> sceneSystems;
		ObjectManager objectManager;

	public:
		Scene(std::unordered_map<std::type_index, AnyRef> externalSystems) :
			externalSystems{ externalSystems },
			objectManager{ *this }
		{

		}

		template<std::derived_from<Object> Ty, class... ConstructorArgs>
		UniqueObject<Ty> NewObject(ConstructorArgs&&... args)
		{
			return objectManager.NewObject<Ty>(std::forward<ConstructorArgs>(args)...);
		}

		void DeleteObject(Object* ptr)
		{
			objectManager.DeleteObject(ptr);
		}

		template<std::invocable Func>
		void LockLifetimes(Func func)
		{
			objectManager.LockLifetimes(func);
		}

		template<std::derived_from<Object> Ty, std::invocable<Ty&> Func>
		void ForEachExactType(Func func)
		{
			objectManager.ForEachExactType<Ty>(func);
		}

		template<std::derived_from<Object> Ty, std::invocable<Ty&> Func>
		void ForEach(Func func)
		{
			objectManager.ForEach<Ty>(func);
		}

		template<class Ty, std::invocable<Ty&> Func>
		void ForEachInterface(Func func)
		{
			objectManager.ForEachInterface<Ty>(func);
		}

		void FlushLifetimes()
		{
			objectManager.FlushLifetimes();
		}

		template<class Ty>
		Ty& GetExternalSystem()
		{
			return externalSystems.at(typeid(Ty)).As<Ty>();
		}

		template<class Ty>
		const Ty& GetExternalSystem() const
		{
			return externalSystems.at(typeid(Ty)).As<Ty>();
		}

		template<std::derived_from<SceneSystem> Ty, class... ConstructorArgs>
		Ty& AddSceneSystem(ConstructorArgs&&... args)
		{
			auto it = sceneSystems.insert({ typeid(Ty), std::make_unique<Ty>(*this, std::forward<ConstructorArgs>(args)...) });
			if(!it.second)
				throw std::exception{};

			return static_cast<Ty&>(*it.first->second);
		}

		template<std::derived_from<SceneSystem> Ty>
		Ty& GetSceneSystem()
		{
			return static_cast<Ty&>(*sceneSystems.at(typeid(Ty)));
		}

		template<std::derived_from<SceneSystem> Ty>
		const Ty& GetSceneSystem() const
		{
			return static_cast<const Ty&>(*sceneSystems.at(typeid(Ty)));
		}

		AnyRef GetSystem(const std::type_info& type) final
		{
			if(externalSystems.contains(type))
				return externalSystems.at(type);
			return sceneSystems.at(type);
		}

		AnyConstRef GetSystem(const std::type_info& type) const final
		{
			if(externalSystems.contains(type))
				return externalSystems.at(type);
			return sceneSystems.at(type);
		}
	};
}