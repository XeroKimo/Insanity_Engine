module;

#include <vector>
#include <cassert>
#include <span>
#include <functional>

export module InsanityFramework.ECS;
export import xk.Math;

namespace InsanityFramework
{
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


}