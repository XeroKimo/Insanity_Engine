module;

#include <cstdint>
#include <memory>
#include <concepts>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <algorithm>
#include <span>
#include <functional>
#include <list>

export module InsanityFramework.ECS.Scene;
import InsanityFramework.Memory;
import InsanityFramework.Allocator;
export import InsanityFramework.ECS.Object;
export import InsanityFramework.TransformationNode;

namespace InsanityFramework
{
	export class Scene;
	export class SceneAllocator;
	export class GameObject;

	struct SceneObjectDeleter;

	export template<std::derived_from<InsanityFramework::Object> Ty>
	using SceneUniqueObject = UniqueObject<Ty, SceneObjectDeleter>;

	export template<std::derived_from<InsanityFramework::GameObject> Ty>
	class UniqueGameObject;

	export class GameObject : public Object, public TransformNode
	{
		template<std::derived_from<InsanityFramework::GameObject> Ty>
		friend class UniqueGameObject;

	public:
		using Object::Object;

		GameObject(Key key, TransformNode* parent) :
			Object{ key },
			TransformNode{ parent }
		{
		}

		GameObject(Key key, LocalTransformInitializer initializer) :
			Object{ key },
			TransformNode{ initializer }
		{
		}

		GameObject(Key key, WorldTransformInitializer initializer) :
			Object{ key },
			TransformNode{ initializer }
		{
		}

	private:
		bool isRoot = true;

	public:
		bool IsRoot() const noexcept { return isRoot; }
	};

	export class SceneSystem : public Object
	{
	protected:
		using Object::Object;
	};

	export struct SceneCallbacks
	{
		virtual void OnObjectCreated(Object* object) {}
		virtual void OnObjectDestroyed(Object* object) {}
	};
	export SceneCallbacks defaultSceneCallbacks;

	export class SceneGroup;

	class Scene
	{
		friend SceneAllocator;
	public:
		struct Key
		{
			friend SceneAllocator;

			friend class SceneGroup;
		private:
			Key() = default;
		};

	private:
		ObjectAllocator allocator;

		std::unordered_map<std::type_index, std::vector<GameObject*>> gameObjects;
		std::unordered_map<std::type_index, UniqueObject<SceneSystem>> sceneSystems;
		std::vector<GameObject*> queuedConstruction;
		std::vector<GameObject*> queuedDestruction;
		std::uint32_t lifetimeLockCounter = 0;

	public:
		inline static SceneCallbacks* callbacks = &defaultSceneCallbacks;
		static Scene* GetActiveScene();
	public:
		Scene(Key) :
			allocator{ /*this*/ }
		{

		}

		~Scene()
		{
			assert(lifetimeLockCounter == 0);
			std::vector<GameObject*> rootObjects;
			for(auto& [type, objects] : gameObjects)
			{
				for(GameObject* object : objects)
				{
					if(object->IsRoot())
						rootObjects.push_back(object);
				}
			}

			for(GameObject* object : rootObjects)
			{
				allocator.Delete(object);
			}

			for(auto& [type, object] : sceneSystems)
			{
				allocator.Delete(object.release());
			}
		}

		bool Contains(Object* object) const
		{
			return allocator.Contains(object);
		}

		//Creates an object that is not registered with the scene
		//Must be paired with DeleteObject
		template<std::derived_from<Object> Ty, class... Args>
		static SceneUniqueObject<Ty> NewObject(Args&&... args)
		{
			SceneUniqueObject<Ty> object = GetActiveScene()->allocator.New<Ty>(std::forward<Args>(args)...);

			callbacks->OnObjectCreated(object.get());

			return object;
		}

		//Creates a game object registered with the scene
		//Must be paired with DeleteGameObject
		template<std::derived_from<GameObject> Ty, class... Args>
		static UniqueGameObject<Ty> NewGameObject(Args&&... args)
		{
			UniqueGameObject<Ty> object = GetActiveScene()->allocator.New<Ty>(std::forward<Args>(args)...);
			if(GetActiveScene()->lifetimeLockCounter == 0)
				GetActiveScene()->gameObjects[typeid(Ty)].push_back(object.get());
			else
				GetActiveScene()->queuedConstruction.push_back(object.get());

			callbacks->OnObjectCreated(object.get());

			return object;
		}

		static void DeleteObject(Object* object)
		{
			Scene* owner = GetOwner(object);
			//Don't delete an object owned by another scene.
			//assert(ObjectAllocator::Get(object) == &owner->allocator);

			//Don't call DeleteObject on managed game objects, call DeleteGameObject instead
			assert(!(owner->gameObjects.contains(typeid(*object)) &&
				std::ranges::find(owner->gameObjects[typeid(*object)], object) != owner->gameObjects[typeid(*object)].end()));

			//Don't call DeleteObject on scene systems, they are expected to have the same lifetime as 
			//the scene itself
			assert(!(owner->sceneSystems.contains(typeid(*object)) && owner->sceneSystems[typeid(*object)].get() == object));

			callbacks->OnObjectDestroyed(object);

			owner->allocator.Delete(object);
		}

		static void DeleteGameObject(GameObject* object)
		{
			Scene* owner = GetOwner(object);
			//Don't delete an object owned by another scene.
			//assert(ObjectAllocator::Get(object) == &owner->allocator);

			if(owner->lifetimeLockCounter == 0)
			{
				owner->ImmediateDeleteGameObject(object);
			}
			else
			{
				owner->queuedDestruction.push_back(object);
			}
		}

		template<std::derived_from<SceneSystem> Ty, class... Args>
		static Ty& AddSystem(Args&&... args)
		{
			auto it = GetActiveScene()->sceneSystems.find(typeid(Ty));
			if(it != GetActiveScene()->sceneSystems.end())
			{
				throw std::exception("System already added");
			}
			auto system = GetActiveScene()->allocator.New<Ty>(std::forward<Args>(args)...);
			GetActiveScene()->sceneSystems.insert({ typeid(Ty), system });
			return *system;
		}

		template<std::derived_from<SceneSystem> Ty>
		static Ty& GetSystem()
		{
			return dynamic_cast<Ty&>(*GetActiveScene()->sceneSystems.at(typeid(Ty)));
		}

		template<std::derived_from<SceneSystem> Ty>
		static Ty* TryGetSystem()
		{
			auto it = GetActiveScene()->sceneSystems.find(typeid(Ty));
			if(it == GetActiveScene()->sceneSystems.end())
			{
				return nullptr;
			}

			return dynamic_cast<Ty*>(it->second);
		}

		//template<std::derived_from<SceneSystem> Ty>
		//static const Ty& GetSystem() const
		//{
		//	return dynamic_cast<const Ty&>(*GetActiveScene()->sceneSystems.at(typeid(Ty)));
		//}

		//template<std::derived_from<SceneSystem> Ty>
		//static const Ty* TryGetSystem() const
		//{
		//	auto it = GetActiveScene()->sceneSystems.find(typeid(Ty));
		//	if(it == GetActiveScene()->sceneSystems.end())
		//	{
		//		return nullptr;
		//	}

		//	return dynamic_cast<const Ty*>(it->second);
		//}


		template<std::derived_from<GameObject> Ty, std::invocable<Ty&> Func>
		void ForEachExactType(Func func) const
		{
			auto it = gameObjects.find(typeid(Ty));
			if(it == gameObjects.end())
				return;

			//Can't directly use static cast in the event of virtual inheritance
			//This is why we find the offset once and do pointer shenangiens to get it again
			Ty* temp = dynamic_cast<Ty*>(it->second.front());
			auto offset = OffsetOf(it->second.front(), temp);

			assert(static_cast<void*>(temp) == IncrementPointer(it->second.front(), offset));

			for(GameObject* object : it->second)
			{
				func(*static_cast<Ty*>(IncrementPointer(object, offset)));
			}
		}

		template<class Ty, std::invocable<Ty&> Func>
		void ForEach(Func func) const
		{
			for(auto& [type, objectVec] : gameObjects)
			{
				if(objectVec.size() == 0)
					continue;

				Ty* temp = dynamic_cast<Ty*>(objectVec.front());
				if(!temp)
					continue;

				auto offset = OffsetOf(objectVec.front(), temp);
				assert(static_cast<void*>(temp) == IncrementPointer(objectVec.front(), offset));
				for(GameObject* object : objectVec)
				{
					func(*static_cast<Ty*>(IncrementPointer(object, offset)));
				}
			}
		}

		template<std::derived_from<GameObject> Ty>
		std::vector<Ty*> GetGameObjects() const
		{
			std::vector<Ty*> output;

			ForEach<Ty>([&output](Ty& obj)
			{
				output.push_back(&obj);
			});

			return output;
		}

		template<std::derived_from<GameObject> Ty>
		std::vector<Ty*> GetGameObjectsOfExactType() const
		{
			std::vector<Ty*> output;

			ForEachExactType<Ty>([&output](Ty& obj)
			{
				output.push_back(&obj);
			});

			return output;
		}

		void LockLifetimes()
		{
			lifetimeLockCounter++;
		}

		void UnlockLifetimes(bool flush = true)
		{
			lifetimeLockCounter--;
			if(lifetimeLockCounter == 0 && flush)
			{
				FlushLifetimes();
			}
		}

		void FlushLifetimes()
		{
			assert(lifetimeLockCounter == 0);

			for(GameObject* object : queuedConstruction)
			{
				gameObjects[typeid(*object)].push_back(object);
			}
			queuedConstruction.clear();

			for(GameObject* object : queuedDestruction)
			{
				ImmediateDeleteGameObject(object);
			}
			queuedDestruction.clear();
		}

		//static Scene* Get(Object* context)
		//{
		//	return ObjectAllocator::Get(context)->GetUserData().As<Scene>();
		//}

	private:
		void ImmediateDeleteGameObject(GameObject* object)
		{
			callbacks->OnObjectDestroyed(object);
			std::erase(gameObjects[typeid(*object)], object);
			allocator.Delete(object);
		}

		static Scene* GetOwner(Object* object);
	};

	struct SceneHandleDeleter
	{
		void operator()(Scene* scene);
	};
	export using UniqueSceneHandle = std::unique_ptr<Scene, SceneHandleDeleter>;

	export class SceneGroup
	{
		inline static std::list<SceneGroup> groups;

		std::vector<std::unique_ptr<Scene>> scenes;

	private:
		SceneGroup() = default;

	public:
		static SceneGroup* New()
		{
			groups.push_front({});

			return &groups.front();
		}

		static SceneGroup* GetGroup(Scene* scene)
		{
			for (SceneGroup& group : groups)
			{
				if (group.Contains(scene))
					return &group;
			}
			return nullptr;
		}

		static std::pair<Scene*, SceneGroup*> GetScene(Object* object)
		{
			assert((false && "Not implemented"));
			for (SceneGroup& group : groups)
			{
				for (const auto& scene : group.scenes)
				{
					if (scene->Contains(object))
						return { scene.get(), &group };
				}
			}
			return {};
		}

	public:
		UniqueSceneHandle NewScene()
		{
			scenes.push_back(std::make_unique<Scene>(Scene::Key{}));
			return { scenes.back().get(), {} };
		}

		void DeleteScene(Scene* scene)
		{
			std::erase_if(scenes, [=](const auto& s) { return s.get() == scene; });
		}

		bool Contains(Scene* scene) const
		{
			return std::find_if(scenes.begin(), scenes.end(), [=](const auto& s) { return s.get() == scene; }) != scenes.end();
		}
	};



	//export struct SceneDeleter
	//{
	//	void operator()(Scene* scene)
	//	{
	//		SceneAllocator::Delete(scene);
	//	}
	//};

	//export using SceneUniquePtr = std::unique_ptr<Scene, SceneDeleter>;

	struct GameObjectDeleter
	{
		void operator()(GameObject* ptr)
		{
			Scene::DeleteGameObject(ptr);
		}
	};

	export template<std::derived_from<InsanityFramework::GameObject> Ty>
	class UniqueGameObject
	{
		template<std::derived_from<InsanityFramework::GameObject> Ty>
		friend class UniqueGameObject;

	private:
		std::unique_ptr<Ty, GameObjectDeleter> ptr;

	public:
		UniqueGameObject() = default;
		UniqueGameObject(std::nullptr_t) : ptr{ nullptr } {}
		UniqueGameObject(Ty* ptr) : ptr{ ptr }
		{
			if(ptr)
				ptr->isRoot = false;
		}
		UniqueGameObject(const UniqueGameObject&) = delete;
		UniqueGameObject(UniqueGameObject&& other) noexcept :
			ptr{ std::move(other).ptr }
		{

		}

		template<class OtherTy>
			requires std::convertible_to<OtherTy*, Ty*>
		UniqueGameObject(UniqueGameObject<OtherTy>&& other) noexcept :
			ptr{ std::move(other).ptr }
		{

		}

		UniqueGameObject& operator=(std::nullptr_t) { ptr = nullptr; return *this; }
		UniqueGameObject& operator=(const UniqueGameObject&) = delete;
		UniqueGameObject& operator=(UniqueGameObject&& other) noexcept
		{
			UniqueGameObject temp{ std::move(other) };
			swap(temp);
			return *this;
		}

		template<class OtherTy>
			requires std::convertible_to<OtherTy*, Ty*>
		UniqueGameObject& operator=(UniqueGameObject<OtherTy>&& other) noexcept
		{
			UniqueGameObject temp{ std::move(other).ptr };
			swap(temp);
			return *this;
		}

		~UniqueGameObject() = default;

	public:
		auto release()
		{
			ptr->isRoot = true;
			return ptr.release();
		}

		void reset(Ty* newPtr) noexcept
		{
			ptr.reset(newPtr);
			if(newPtr)
				newPtr->isRoot = false;
		}

		void swap(UniqueGameObject& other) noexcept
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

	struct SceneObjectDeleter
	{
		void operator()(Object* ptr)
		{
			Scene::DeleteObject(ptr);
		}
	};

	export class SceneLifetimeScopeLock
	{
	private:
		Scene& scene;

	public:
		SceneLifetimeScopeLock(Scene& scene) :
			scene{ scene }
		{
			scene.LockLifetimes();
		}

		~SceneLifetimeScopeLock()
		{
			scene.UnlockLifetimes();
		}
	};

	void SceneHandleDeleter::operator()(Scene* scene)
	{
		SceneGroup::GetGroup(scene)->DeleteScene(scene);
	}
}