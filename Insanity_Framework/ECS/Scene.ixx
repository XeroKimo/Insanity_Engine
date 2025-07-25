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

	struct SceneCallbacks
	{
		virtual void OnObjectCreated(Object* object) {}
		virtual void OnObjectDestroyed(Object* object) {}
	};
	export SceneCallbacks defaultSceneCallbacks;

	class Scene
	{
		friend SceneAllocator;

	public:
		struct Key
		{
			friend SceneAllocator;

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

	public:
		Scene(Key) :
			allocator{ this }
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

		//Creates an object that is not registered with the scene
		//Must be paired with DeleteObject
		template<std::derived_from<Object> Ty, class... Args>
		SceneUniqueObject<Ty> NewObject(Args&&... args)
		{
			SceneUniqueObject<Ty> object = allocator.New<Ty>(std::forward<Args>(args)...);

			callbacks->OnObjectCreated(object.get());

			return object;
		}

		//Creates a game object registered with the scene
		//Must be paired with DeleteGameObject
		template<std::derived_from<GameObject> Ty, class... Args>
		UniqueGameObject<Ty> NewGameObject(Args&&... args)
		{
			UniqueGameObject<Ty> object = allocator.New<Ty>(std::forward<Args>(args)...);
			if(lifetimeLockCounter == 0)
				gameObjects[typeid(Ty)].push_back(object.get());
			else
				queuedConstruction.push_back(object.get());

			callbacks->OnObjectCreated(object.get());

			return object;
		}

		void DeleteObject(Object* object)
		{
			//Don't delete an object owned by another scene.
			assert(ObjectAllocator::Get(object) == &allocator);

			//Don't call DeleteObject on managed game objects, call DeleteGameObject instead
			assert(!(gameObjects.contains(typeid(*object)) &&
				std::ranges::find(gameObjects[typeid(*object)], object) != gameObjects[typeid(*object)].end()));

			//Don't call DeleteObject on scene systems, they are expected to have the same lifetime as 
			//the scene itself
			assert(!(sceneSystems.contains(typeid(*object)) && sceneSystems[typeid(*object)].get() == object));

			callbacks->OnObjectDestroyed(object);

			allocator.Delete(object);
		}

		void DeleteGameObject(GameObject* object)
		{
			//Don't delete an object owned by another scene.
			assert(ObjectAllocator::Get(object) == &allocator);

			if(lifetimeLockCounter == 0)
			{
				ImmediateDeleteGameObject(object);
			}
			else
			{
				queuedDestruction.push_back(object);
			}
		}

		template<std::derived_from<SceneSystem> Ty, class... Args>
		Ty& AddSystem(Args&&... args)
		{
			auto it = sceneSystems.find(typeid(Ty));
			if(it != sceneSystems.end())
			{
				throw std::exception("System already added");
			}
			auto system = allocator.New<Ty>(std::forward<Args>(args)...);
			sceneSystems.insert({ typeid(Ty), system });
			return *system;
		}

		template<std::derived_from<SceneSystem> Ty>
		Ty& GetSystem()
		{
			return dynamic_cast<Ty&>(*sceneSystems.at(typeid(Ty)));
		}

		template<std::derived_from<SceneSystem> Ty>
		Ty* TryGetSystem()
		{
			auto it = sceneSystems.find(typeid(Ty));
			if(it == sceneSystems.end())
			{
				return nullptr;
			}

			return dynamic_cast<Ty*>(it->second);
		}

		template<std::derived_from<SceneSystem> Ty>
		const Ty& GetSystem() const
		{
			return dynamic_cast<const Ty&>(*sceneSystems.at(typeid(Ty)));
		}

		template<std::derived_from<SceneSystem> Ty>
		const Ty* TryGetSystem() const
		{
			auto it = sceneSystems.find(typeid(Ty));
			if(it == sceneSystems.end())
			{
				return nullptr;
			}

			return dynamic_cast<const Ty*>(it->second);
		}


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

		static Scene* Get(Object* context)
		{
			return ObjectAllocator::Get(context)->GetUserData().As<Scene>();
		}

	private:
		void ImmediateDeleteGameObject(GameObject* object)
		{
			callbacks->OnObjectDestroyed(object);
			std::erase(gameObjects[typeid(*object)], object);
			allocator.Delete(object);
		}

	private:
		void* operator new(std::size_t size, SceneAllocator& allocator);

		void operator delete(void* ptr, SceneAllocator& allocator);

		void operator delete(void* ptr);
	};

	export class SceneAllocator
	{
		friend Scene;

		class Page : public IntrusiveForwardListNode<Page>
		{
		public:
			static constexpr size_t minimumBlocksPerPage = 16;

			static constexpr size_t PageSize()
			{
				return AlignNextPow2(sizeof(Page) + PoolAllocator<sizeof(Scene)>::alignedBucketSize * minimumBlocksPerPage);
			}

			static Page* GetPageFrom(void* ptr)
			{
				return std::launder(static_cast<Page*>(AlignFloorPow2(ptr, PageSize())));
			}
		private:
			SceneAllocator* owningAllocator;
			PoolAllocator<sizeof(Scene)> allocator;

		public:
			Page(SceneAllocator* owner) :
				owningAllocator{ owner },
				allocator{ { this + 1, PageSize() - sizeof(Page) } }
			{

			}		
			
			void* Allocate()
			{
				return allocator.Allocate(sizeof(Scene));
			}

			void Free(void* ptr)
			{
				allocator.Free(ptr);
			}

			SceneAllocator* GetOwner() const { return owningAllocator; }

		public:
			void* operator new(size_t size)
			{
				return ::operator new(PageSize(), std::align_val_t{ PageSize() });
			}

			void operator delete(void* ptr)
			{
				::operator delete(ptr, std::align_val_t{ PageSize() });
			}
		};

	private:
		AnyPtr userData = nullptr;
		Page* firstPage = new Page(this);

	public:
		SceneAllocator() = default;
		SceneAllocator(AnyPtr userData) :
			userData{ userData }
		{

		}

		~SceneAllocator()
		{
			while(firstPage)
			{
				delete std::exchange(firstPage, firstPage->RemoveSelfAndGetNext());
			}
		}

		Scene* New()
		{
			return new(*this) Scene{ Scene::Key{} };
		}

		static void Delete(Scene* scene)
		{
			delete scene;
		}

		void SetUserData(AnyPtr ptr)
		{
			userData = ptr;
		}

		AnyPtr GetUserData() const
		{
			return userData;
		}

		static SceneAllocator* Get(Scene* ptr)
		{
			return Page::GetPageFrom(ptr)->GetOwner();
		}

	private:
		void* Allocate()
		{
			Page* currentPage = firstPage;
			void* address = currentPage->Allocate();

			while(!address)
			{
				Page* oldPage = currentPage;
				currentPage = currentPage->Next();

				if(!currentPage)
				{
					currentPage = new Page{ this };
					oldPage->Append(currentPage);
				}

				address = currentPage->Allocate();
			}

			return address;
		}

		static void Free(void* ptr)
		{
			Page::GetPageFrom(ptr)->Free(ptr);
		}
	};



	void* Scene::operator new(std::size_t size, SceneAllocator& allocator)
	{
		return allocator.Allocate();
	}
	void Scene::operator delete(void* ptr, SceneAllocator& allocator)
	{
		allocator.Free(ptr);
	}
	void Scene::operator delete(void* ptr)
	{
		SceneAllocator::Free(ptr);
	}



	export struct SceneDeleter
	{
		void operator()(Scene* scene)
		{
			SceneAllocator::Delete(scene);
		}
	};

	export using SceneUniquePtr = std::unique_ptr<Scene, SceneDeleter>;

	struct GameObjectDeleter
	{
		void operator()(GameObject* ptr)
		{
			Scene::Get(ptr)->DeleteGameObject(ptr);
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
			Scene::Get(ptr)->DeleteObject(ptr);
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
}