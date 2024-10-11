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

	export struct LocalTransformType : Transform {};
	export struct WorldTransformType : Transform {};

	enum class TransformDestructorLogic
	{
		Null_Parent_Keep_Local_Transform,
		Null_Parent_Keep_World_Transform,
		Reparent_Keep_Local_Transform,
		Reparent_Keep_World_Transform,
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

		TransformNode(TransformNode* parent)
		{
			SetParent(parent);
		}

		TransformNode(LocalTransformType transform, TransformNode* parent)
		{
			SetParent(parent);
			SetLocalTransform(transform);
		}

		TransformNode(WorldTransformType transform, TransformNode* parent)
		{
			SetParent(parent);
			SetWorldTransform(transform);
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
			class PositionProxy
			{
				using proxy_type = PositionProxy;
				inline static const auto Setter = &TransformNode::SetLocalPosition;
				inline static const auto Getter = &TransformNode::GetLocalPosition;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				PositionProxy(TransformNode* node) :
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

				operator value_type() const { return Get(); }
			};

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

			class ScaleProxy
			{
				using proxy_type = ScaleProxy;
				inline static const auto Setter = &TransformNode::SetLocalScale;
				inline static const auto Getter = &TransformNode::GetLocalScale;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				ScaleProxy(TransformNode* node) :
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

				operator value_type() const { return Get(); }
			};


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
			class PositionProxy
			{
				using proxy_type = PositionProxy;
				inline static const auto Setter = &TransformNode::SetWorldPosition;
				inline static const auto Getter = &TransformNode::GetWorldPosition;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				PositionProxy(TransformNode* node) :
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

				operator value_type() const { return Get(); }
			};

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

			class ScaleProxy
			{
				using proxy_type = ScaleProxy;
				inline static const auto Setter = &TransformNode::SetWorldScale;
				inline static const auto Getter = &TransformNode::GetWorldScale;
				using value_type = decltype(std::invoke(Getter, std::declval<TransformNode*>()));
				using underlying_type = value_type::value_type;

			private:
				TransformNode* node;

			public:
				void Set(value_type value) { std::invoke(Setter, node, value); }
				value_type Get() const { return std::invoke(Getter, node); }

			public:
				ScaleProxy(TransformNode* node) :
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

				operator value_type() const { return Get(); }
			};


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