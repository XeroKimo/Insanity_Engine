module;

#include <vector>
#include <cassert>
#include <span>
#include <functional>
#include <concepts>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <optional>
#include <any>
#include <utility>
#include <tuple>

export module InsanityFramework.ECS;
export import xk.Math;
export import InsanityFramework.AnyRef;

namespace InsanityFramework
{
	export class Object;
	export class GameObject;

	export class Scene;
	export enum class VirtualConstructorPassthrough {};

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

	
	template<bool IsConst, auto Setter, auto Getter>
	class TransformProxyType;

	template<bool IsConst, auto Setter, auto Getter, size_t Index>
	class VectorProxyType
	{
		using proxy_type = VectorProxyType<IsConst, Setter, Getter, Index>;

		template<bool OtherConst>
		using proxy_template_type = VectorProxyType<OtherConst, Setter, Getter, Index>;
		using value_type = typename ReturnType<decltype(Getter)>::type::value_type;
		using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;

	private:
		node_pointer_type node;

	public:
		void Set(value_type value) requires(!IsConst)
		{
			auto temp = std::invoke(Getter, node);
			temp[Index] = value;
			std::invoke(Setter, node, temp);
		}
		value_type Get() const { return std::invoke(Getter, node)[Index]; }

	public:
		VectorProxyType(node_pointer_type node) :
			node{ node }
		{

		}

		value_type operator=(const value_type& other) requires(!IsConst)
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

		value_type operator+=(const value_type& other) requires(!IsConst)
		{
			Set(Get() + other);
			return Get();
		}

		value_type operator-=(const value_type& other) requires(!IsConst)
		{
			Set(Get() - other);
			return Get();
		}

		value_type operator*=(const value_type& other) requires(!IsConst)
		{
			Set(Get() * other);
			return Get();
		}

		value_type operator/=(const value_type& other) requires(!IsConst)
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

		template<bool OtherConst>
		value_type operator=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator=(other.Get());
		}

		template<bool OtherConst>
		friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() == rh.Get();
		}

		template<bool OtherConst>
		value_type operator+=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator+=(other.Get());
		}

		template<bool OtherConst>
		value_type operator-=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator-=(other.Get());
		}

		template<bool OtherConst>
		value_type operator*=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator*=(other.Get());
		}

		template<bool OtherConst>
		value_type operator/=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator/=(other.Get());
		}

		template<bool OtherConst>
		friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() + rh;
		}

		template<bool OtherConst>
		friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() - rh;
		}

		template<bool OtherConst>
		friend value_type operator*(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() * rh;
		}

		template<bool OtherConst>
		friend value_type operator/(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() / rh;
		}

		operator value_type() const { return Get(); }
	};

	template<bool IsConst, auto Setter, auto Getter>
		requires std::same_as<typename ReturnType<decltype(Getter)>::type, xk::Math::Vector<float, 3>>
	class TransformProxyType<IsConst, Setter, Getter>
	{
		using proxy_type = TransformProxyType<IsConst, Setter, Getter>;

		template<bool OtherConst>
		using proxy_template_type = TransformProxyType<OtherConst, Setter, Getter>;
		using value_type = typename ReturnType<decltype(Getter)>::type;
		using underlying_type = value_type::value_type;
		using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;

	private:
		node_pointer_type node;

	public:
		void Set(value_type value) requires(!IsConst) { std::invoke(Setter, node, value); }
		value_type Get() const { return std::invoke(Getter, node); }

	public:
		TransformProxyType(node_pointer_type node) :
			node{ node }
		{

		}

		value_type operator=(const value_type& other) requires(!IsConst)
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

		value_type operator+=(const value_type& other) requires(!IsConst)
		{
			Set(Get() + other);
			return Get();
		}

		value_type operator-=(const value_type& other) requires(!IsConst)
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

		template<bool OtherConst>
		value_type operator=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator=(other.Get());
		}

		template<bool OtherConst>
		friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() == rh.Get();
		}

		template<bool OtherConst>
		value_type operator+=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator+=(other.Get());
		}

		template<bool OtherConst>
		value_type operator-=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
		{
			return operator-=(other.Get());
		}

		//value_type operator*=(const proxy_template_type<OtherConst>& other)
		//{
		//	return operator*=(other.Get());
		//}

		//value_type operator/=(const proxy_template_type<OtherConst>& other)
		//{
		//	return operator/=(other.Get());
		//}

		template<bool OtherConst>
		friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() + rh;
		}

		template<bool OtherConst>
		friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		{
			return lh.Get() - rh;
		}

		//friend value_type operator*(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
		//{
		//	return lh.Get() * rh;
		//}

		//friend value_type operator/(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
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

		VectorProxyType<IsConst, Setter, Getter, 0> X() const { return { node }; };
		VectorProxyType<IsConst, Setter, Getter, 1> Y() const { return { node }; };
		VectorProxyType<IsConst, Setter, Getter, 2> Z() const { return { node }; };

		template<size_t... Index>
		auto Swizzle() const { return Get().Swizzle<Index...>(); }

		operator value_type() const { return Get(); }
	};

	export class TransformNode
	{
	private:
		template<bool IsConst>
		class LocalTransformProxy;

		template<bool IsConst>
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

		LocalTransformProxy<false> LocalTransform()
		{
			return { this };
		}

		LocalTransformProxy<true> LocalTransform() const
		{
			return { this };
		}

		WorldTransformProxy<false> WorldTransform()
		{
			return { this };
		}

		WorldTransformProxy<true> WorldTransform() const
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

		template<bool IsConst>
		class LocalTransformProxy
		{
			using PositionProxy = TransformProxyType<IsConst, &TransformNode::SetLocalPosition, &TransformNode::GetLocalPosition>;
			template<bool IsConst>
			class RotationProxyType
			{
				using proxy_type = RotationProxyType<IsConst>;

				template<bool OtherConst>
				using proxy_template_type = RotationProxyType<OtherConst>;

				inline static const auto Setter = &TransformNode::SetLocalRotation;
				inline static const auto Getter = &TransformNode::GetLocalRotation;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;			
				using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;


			private:
				node_pointer_type node;

			public:
				void Set(value_type value) requires(!IsConst){ std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				RotationProxyType(node_pointer_type node) :
					node{ node }
				{

				}

				value_type operator=(const value_type& other) requires(!IsConst)
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

				value_type operator+=(const value_type& other) requires(!IsConst)
				{
					Set(Get() + other);
					return Get();
				}

				value_type operator-=(const value_type& other) requires(!IsConst)
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

				template<bool OtherConst>
				value_type operator=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator=(other.Get());
				}

				template<bool OtherConst>
				friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() == rh.Get();
				}

				template<bool OtherConst>
				value_type operator+=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator+=(other.Get());
				}

				template<bool OtherConst>
				value_type operator-=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator-=(other.Get());
				}

				//value_type operator*=(const proxy_template_type<OtherConst>& other)
				//{
				//	return operator*=(other.Get());
				//}

				//value_type operator/=(const proxy_template_type<OtherConst>& other)
				//{
				//	return operator/=(other.Get());
				//}

				template<bool OtherConst>
				friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() + rh;
				}

				template<bool OtherConst>
				friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() - rh;
				}

				//friend value_type operator*(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator/(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
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
			using RotationProxy = RotationProxyType<IsConst>;
			using ScaleProxy = TransformProxyType<IsConst, &TransformNode::SetLocalScale, &TransformNode::GetLocalScale>;


			using proxy_type = LocalTransformProxy;
			template<bool OtherConst>
			using proxy_template_type = LocalTransformProxy<OtherConst>;
			inline static const auto Setter = &TransformNode::SetLocalTransform;
			inline static const auto Getter = &TransformNode::GetLocalTransform;
			using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
			using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;

		private:
			node_pointer_type node;

		public:
			void Set(value_type value) requires(!IsConst) { std::invoke(Setter, node, value); }
			value_type Get() const { return std::invoke(Getter, node); }

			PositionProxy Position() const { return PositionProxy{ node }; }
			RotationProxy Rotation() const { return RotationProxy{ node }; }
			ScaleProxy Scale()       const { return ScaleProxy{ node }; }

		public:
			LocalTransformProxy(node_pointer_type node) :
				node{ node }
			{

			}

			value_type operator=(const value_type& other) requires (!IsConst) 
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

			value_type operator+=(const value_type& other) requires (!IsConst)
			{
				Set(Get() + other);
				return Get();
			}

			value_type operator-=(const value_type& other) requires (!IsConst)
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

			template<bool OtherConst>
			value_type operator=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator=(other.Get());
			}

			template<bool OtherConst>
			friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
			{
				return lh.Get() == rh.Get();
			}

			template<bool OtherConst>
			value_type operator+=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator+=(other.Get());
			}

			template<bool OtherConst>
			value_type operator-=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator-=(other.Get());
			}

			template<bool OtherConst>
			friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
			{
				return lh.Get() + rh;
			}

			template<bool OtherConst>
			friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
			{
				return lh.Get() - rh;
			}

			operator value_type() const { return Get(); }
		};


		template<bool IsConst>
		class WorldTransformProxy
		{
			using PositionProxy = TransformProxyType<IsConst, &TransformNode::SetWorldPosition, &TransformNode::GetWorldPosition>;
			template<bool IsConst>
			class RotationProxyType
			{
				using proxy_type = RotationProxyType<IsConst>;

				template<bool OtherConst>
				using proxy_template_type = RotationProxyType<OtherConst>;

				inline static const auto Setter = &TransformNode::SetWorldRotation;
				inline static const auto Getter = &TransformNode::GetWorldRotation;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;
				using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;


			private:
				node_pointer_type node;

			public:
				void Set(value_type value)  { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				RotationProxyType(node_pointer_type node) :
					node{ node }
				{

				}

				value_type operator=(const value_type& other) requires(!IsConst)
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

				value_type operator+=(const value_type& other) requires(!IsConst)
				{
					Set(Get() + other);
					return Get();
				}

				value_type operator-=(const value_type& other) requires(!IsConst)
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

				template<bool OtherConst>
				value_type operator=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator=(other.Get());
				}

				template<bool OtherConst>
				friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() == rh.Get();
				}

				template<bool OtherConst>
				value_type operator+=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator+=(other.Get());
				}

				template<bool OtherConst>
				value_type operator-=(const proxy_template_type<OtherConst>& other) requires(!IsConst)
				{
					return operator-=(other.Get());
				}

				//value_type operator*=(const proxy_template_type<OtherConst>& other)
				//{
				//	return operator*=(other.Get());
				//}

				//value_type operator/=(const proxy_template_type<OtherConst>& other)
				//{
				//	return operator/=(other.Get());
				//}

				template<bool OtherConst>
				friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() + rh;
				}

				template<bool OtherConst>
				friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				{
					return lh.Get() - rh;
				}

				//friend value_type operator*(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
				//{
				//	return lh.Get() * rh;
				//}

				//friend value_type operator/(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
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
			using RotationProxy = RotationProxyType<IsConst>;
			using ScaleProxy = TransformProxyType<IsConst, &TransformNode::SetWorldScale, &TransformNode::GetWorldScale>;

			using proxy_type = WorldTransformProxy;

			template<bool OtherConst>
			using proxy_template_type = WorldTransformProxy<OtherConst>;

			inline static const auto Setter = &TransformNode::SetWorldTransform;
			inline static const auto Getter = &TransformNode::GetWorldTransform;
			using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
			using node_pointer_type = std::conditional_t<IsConst, const TransformNode*, TransformNode*>;

		private:
			node_pointer_type node;

		public:
			void Set(value_type value) requires (!IsConst) { std::invoke(Setter, node, value); }
			value_type Get() const { return std::invoke(Getter, node); }

			PositionProxy Position() const { return PositionProxy{ node }; }
			RotationProxy Rotation() const { return RotationProxy{ node }; }
			ScaleProxy Scale() const { return ScaleProxy{ node }; }

		public:
			WorldTransformProxy(node_pointer_type node) :
				node{ node }
			{

			}

			value_type operator=(const value_type& other) requires (!IsConst)
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

			value_type operator+=(const value_type& other) requires (!IsConst)
			{
				Set(Get() + other);
				return Get();
			}

			value_type operator-=(const value_type& other) requires (!IsConst)
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

			template<bool OtherConst>
			value_type operator=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator=(other.Get());
			}

			template<bool OtherConst>
			friend bool operator==(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
			{
				return lh.Get() == rh.Get();
			}

			template<bool OtherConst>
			value_type operator+=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator+=(other.Get());
			}

			template<bool OtherConst>
			value_type operator-=(const proxy_template_type<OtherConst>& other) requires (!IsConst)
			{
				return operator-=(other.Get());
			}

			template<bool OtherConst>
			friend value_type operator+(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
			{
				return lh.Get() + rh;
			}

			template<bool OtherConst>
			friend value_type operator-(const proxy_type& lh, const proxy_template_type<OtherConst>& rh)
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

	public:
		ObjectManager() = default;
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

	export template<class Ty, class... Params>
	concept IsSceneClass = requires(Ty t, Params... params)
	{
		requires std::constructible_from<Ty, Scene*>;
		t.LoadResources(params...);
		t.LoadSystems(params...);
		t.LoadObjects(params...);
	};

	export class SceneClassFactory
	{
		struct Base
		{
			virtual ~Base() = default;

			virtual MoveOnlyAny CreateClass(Scene* scene) = 0;
		};

		template<class Ty, class... Params>
			requires IsSceneClass<Ty, Params...>
		struct Impl final : Base
		{
			std::tuple<Params&&...> data;

			Impl(Params&&... data) : data{ std::forward<Params>(data)... }
			{

			}

			MoveOnlyAny CreateClass(Scene* scene)
			{
				Ty sceneClass{ scene };

				if constexpr(sizeof...(Params) > 0)
				{
					std::apply([&sceneClass](Params&&... params)
					{
						sceneClass.LoadResources(std::forward<Params>(params...));
						sceneClass.LoadSystems(std::forward<Params>(params...));
						sceneClass.LoadObjects(std::forward<Params>(params...));
					}, data);
				}
				else
				{
					sceneClass.LoadResources();
					sceneClass.LoadSystems();
					sceneClass.LoadObjects();
				}

				return sceneClass;
			}
		};

		std::unique_ptr<Base> ptr;

	public:

		template<class Ty, class... Params>
			requires IsSceneClass<Ty, Params...>
		SceneClassFactory(std::type_identity<Ty>, Params&&... params) :
			ptr{ std::make_unique<Impl<Ty, Params...>>(std::forward<Params>(params)...) }
		{

		}

		MoveOnlyAny CreateClass(Scene* scene)
		{
			return ptr->CreateClass(scene);
		}
	};

	void InternalRegisterGlobalSystem(std::type_index index, AnyRef system);
	void InternalUnregisterGlobalSystem(std::type_index index);
	AnyRef InternalGetGlobalSystem(std::type_index index);

	export template<class Ty>
		void RegisterGlobalSystem(Ty& system)
	{
		InternalRegisterGlobalSystem(typeid(Ty), system);
	}

	export template<class Ty>
		void UnregisterGlobalSystem()
	{
		InternalUnregisterGlobalSystem(typeid(Ty));
	}

	export template<class Ty>
		Ty& GetGlobalSystem()
	{
		return InternalGetGlobalSystem(typeid(Ty)).As<Ty>();
	}

	export class Scene
	{
	private:
		std::unordered_map<std::type_index, std::unique_ptr<SceneSystem>> sceneSystems;
		ObjectManager objectManager;
		MoveOnlyAny sceneClass;

	public:
		Scene(SceneClassFactory factory) :
			sceneClass{ factory.CreateClass(this) }
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
		Ty& GetClass()
		{
			return sceneClass.As<Ty>();
		}

		template<class Ty>
		const Ty& GetClass() const
		{
			return sceneClass.As<Ty>();
		}

		template<class Ty>
		Ty& GetExternalSystem()
		{
			return GetGlobalSystem<Ty>();
		}

		template<class Ty>
		const Ty& GetExternalSystem() const
		{
			return GetGlobalSystem<Ty>();
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

		AnyRef GetSystem(const std::type_info& type)
		{
			if(sceneSystems.contains(type))
				return sceneSystems.at(type);
			return InternalGetGlobalSystem(type);
		}

		AnyConstRef GetSystem(const std::type_info& type) const
		{
			if(sceneSystems.contains(type))
				return sceneSystems.at(type);
			return InternalGetGlobalSystem(type);
		}
	};

	export class SceneManager
	{
	private:
		std::unique_ptr<Scene> scene;
		std::optional<SceneClassFactory> pendingScene;

	public:
		template<class Ty, class... Params>
		void LoadScene(Params&&... params)
		{
			LoadScene(SceneClassFactory(std::type_identity<Ty>{}, std::forward<Params>(params)... ));
		}

		void LoadScene(SceneClassFactory factory)
		{
			pendingScene = std::move(factory);
		}

		void WaitForScene()
		{
			scene = nullptr;
			scene = std::make_unique<Scene>(std::move(pendingScene.value()));
			pendingScene = std::nullopt;
		}

		bool HasPendingLoadScene()
		{
			return pendingScene.has_value();
		}

		Scene& GetActiveScene() { return *scene; }
		const Scene& GetActiveScene() const { return *scene; }
	};
}


module:private;

namespace InsanityFramework
{
	std::unordered_map<std::type_index, AnyRef> systems;

	void InternalRegisterGlobalSystem(std::type_index index, AnyRef system)
	{
		systems.insert({ index, system });
	}

	void InternalUnregisterGlobalSystem(std::type_index index)
	{
		systems.erase(index);
	}

	AnyRef InternalGetGlobalSystem(std::type_index index)
	{
		return systems.at(index);
	}
}