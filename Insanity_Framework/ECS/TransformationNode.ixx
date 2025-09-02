module;

#include <vector>
#include <span>
#include <cassert>

export module InsanityFramework.TransformationNode;
import xk.Math;

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

		xk::Math::Matrix<float, 4, 4> ToMatrix() const noexcept
		{
			return xk::Math::ScaleMatrix(scale) * xk::Math::TransformMatrix(position);
		}

		static Transform FromMatrix(const xk::Math::Matrix<float, 4, 4>& matrix)
		{
			return
			{
				{ matrix.At(0, 3), matrix.At(1, 3), matrix.At(2, 3) },
				{},
				{ matrix.At(0, 0), matrix.At(1, 1), matrix.At(2, 2) }
			};
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
				void Set(value_type value) requires(!IsConst) { std::invoke(Setter, node, value); }
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
				void Set(value_type value) { std::invoke(Setter, node, value); }
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
}
