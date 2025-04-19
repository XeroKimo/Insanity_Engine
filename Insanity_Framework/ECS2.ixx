module;

#include <optional>
#include <new>
#include <cassert>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <memory>

export module InsanityFramework.ECS2;
import InsanityFramework.Memory;
import InsanityFramework.Allocator;

namespace InsanityFramework
{

	template<class Ty>
	struct ConstructorKey
	{
		friend Ty;
	private:
		ConstructorKey() = default;
	};

	export class Scene;
	export class SceneManager;

	class ObjectAllocator;
	using ObjectConstructorKey = ConstructorKey<Scene>;

	export class Object
	{
		
	public:
		Object(ObjectConstructorKey) {}

		virtual ~Object() = default;

		void* operator new(std::size_t size, ObjectAllocator& allocator);
		void operator delete(void* ptr, ObjectAllocator& allocator);
		void operator delete(void* ptr);
	};


	class SceneAllocator;

	struct SceneConstructorKey
	{
		friend SceneManager;
		SceneManager* manager;
	private:
		SceneConstructorKey(SceneManager* manager) : manager{ manager } {}
	};

	class ObjectPage : public IntrusiveForwardListNode<ObjectPage>
	{
		static constexpr size_t pageSize = AlignNextPow2<size_t>(8'000'000);


		SceneManager* sceneManager;
		Scene* scene;
		FreeListAllocator allocator;


	public:
		void* operator new(size_t size)
		{
			return ::operator new(pageSize, std::align_val_t{ pageSize });
		}

		void operator delete(void* ptr)
		{
			::operator delete(ptr, std::align_val_t{ pageSize });
		}

	public:
		ObjectPage(SceneManager* manager, Scene* scene) :
			sceneManager{ manager },
			scene{ scene },
			allocator{ {IncrementPointerAs<ObjectPage>(this, 1), pageSize - sizeof(ObjectPage) } }
		{
		}

		void* Allocate(size_t requestedSize)
		{
			return allocator.Allocate(requestedSize);
		}

		void Free(void* pointer)
		{
			allocator.Free(pointer);
		}

		SceneManager* GetSceneManager() const { return sceneManager; }
		Scene* GetScene() const { return scene; }

		static ObjectPage* GetPageFromPointer(void* ptr)
		{
			return std::launder(static_cast<ObjectPage*>(AlignFloorPow2(ptr, pageSize)));
		}
	};


	class ObjectAllocator
	{
		ObjectPage* pages = nullptr;

	public:
		ObjectAllocator(SceneManager* manager, Scene* scene) :
			pages{ new ObjectPage(manager, scene) }
		{

		}
		~ObjectAllocator()
		{
			while(pages)
			{
				delete std::exchange(pages, pages->Next());
			}
		}

		void* Allocate(size_t size)
		{
			ObjectPage* currentPage = pages;
			void* ptr = currentPage->Allocate(size);
			while(!ptr)
			{
				ObjectPage* oldPage = currentPage;
				currentPage = currentPage->Next();
				if(!currentPage)
				{
					currentPage = new ObjectPage(oldPage->GetSceneManager(), oldPage->GetScene());
					oldPage->Append(currentPage);
				}
				ptr = currentPage->Allocate(size);
			}

			return ptr;
		}

		void Free(void* ptr)
		{
			ObjectPage::GetPageFromPointer(ptr)->Free(ptr);
		}
	};

	class Scene
	{
		ObjectAllocator objectAllocator;

	public:
		Scene(SceneConstructorKey key) :
			objectAllocator{ key.manager, this }
		{
		}

		template<std::derived_from<Object> Ty, class... ConstructorTy>
		Ty* NewUnregisteredObject(ConstructorTy&&... args)
		{
			return new(objectAllocator) Ty{ ObjectConstructorKey{}, std::forward<ConstructorTy>(args)... };
		}

		template<std::derived_from<Object> Ty, class... ConstructorTy>
		Ty* NewObject(ConstructorTy&&... args)
		{
			Ty* ptr = NewUnregisteredObject<Ty>(std::forward<ConstructorTy>(args)...);

			return ptr;
		}

		void DeleteUnregisteredObject(Object* ptr)
		{
			delete ptr;
		}
	};

	class ScenePage
	{
	public:
		static constexpr size_t minimumBlocksPerPage = 16;

		static constexpr size_t PageSize()
		{
			return AlignNextPow2(sizeof(ScenePage) + sizeof(PoolAllocator<sizeof(Scene)>::alignedBucketSize) * minimumBlocksPerPage + 1);
		}

		static ScenePage* GetPageFromPointer(void* ptr)
		{
			return std::launder(static_cast<ScenePage*>(AlignFloorPow2(ptr, PageSize())));
		}

	private:
		ScenePage* next;
		SceneManager* manager;
		PoolAllocator<sizeof(Scene)> allocator;

	public:
		ScenePage(SceneManager* manager) :
			manager{ manager },
			allocator{ { this + 1, PageSize() - sizeof(ScenePage) } }
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

		void AppendPage(ScenePage* page) noexcept
		{
			if(next)
				page->next = next->next;

			next = page;
		}

		ScenePage* NextPage() const noexcept
		{
			return next;
		}

		ScenePage* SpliceNext() noexcept
		{
			if(next)
				return std::exchange(next, next->next);

			return next;
		}

		SceneManager* GetSceneManager() const noexcept { return manager; }

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

	class SceneAllocator
	{
		ScenePage* pages;

	public:
		SceneAllocator(SceneManager* manager) :
			pages(new ScenePage{ manager })
		{

		}

		~SceneAllocator()
		{
			while(pages)
			{
				delete std::exchange(pages, pages->NextPage());
			}
		}

		void* Allocate()
		{
			ScenePage* currentPage = pages;
			void* ptr = pages->Allocate();
			while(!ptr)
			{
				ScenePage* oldPage = currentPage;
				currentPage = currentPage->NextPage();
				if(!currentPage)
				{
					currentPage = new ScenePage(pages->GetSceneManager());
					oldPage->AppendPage(currentPage);
				}

				ptr = currentPage->Allocate();
			}
			return ptr;
		}

		void Free(void* ptr)
		{
			ScenePage::GetPageFromPointer(ptr)->Free(ptr);
		}
	};

	template<class Ty>
	struct Identity {};

	class SceneClass : public Object
	{

	public:
		using Object::Object;

		virtual void Unload() = 0;
	};

	template<class Ty, class... Args>
	concept IsSceneClass = requires(Ty t, Args&&... args)
	{
		requires std::derived_from<Ty, SceneClass>;
		t.Load(std::forward<Args>(args)...);
	};

	class SceneClassFactory
	{
		struct Base
		{
			virtual ~Base() = default;

			virtual SceneClass* Load(Scene* scene) = 0;
		};

		template<IsSceneClass Ty, class... Args>
		struct Derived
		{
			std::tuple<Args...> args;
			Derived(Args&&... data) : args{ std::forward<Args>(data)... }
			{

			}

			SceneClass* Load(Scene* scene)
			{
				Ty* sceneClass = scene->NewUnregisteredObject<Ty>();
				if constexpr(sizeof...(Args) > 0)
				{
					std::apply([&sceneClass](Args&&... params)
					{
						sceneClass->Load(std::forward<Args>(params)...);
					}, args);
				}
				else
				{
					sceneClass->Load();
				}
				return sceneClass;
			}
		};

		std::unique_ptr<Base> ptr;

	public:
		template<IsSceneClass Ty, class... Args>
		SceneClassFactory(std::type_identity<Ty>, Args&&... params) :
			ptr{ std::make_unique<Derived<Ty, Args...>>(std::forward<Args>(params)...) }
		{

		}

		SceneClass* Load(Scene* scene)
		{
			return ptr->Load(scene);
		}
	};

	class SceneManager
	{
		struct LoadedScenes
		{
			Scene* scene;
			SceneClass* sceneClass;
		};

	private:
		SceneAllocator sceneAllocator;
		std::vector<LoadedScenes> scenes;
		std::optional<SceneClassFactory> pendingSceneLoader;

	public:

		~SceneManager()
		{
			UnloadScene();
		}


		void LoadScene();
		void LoadSubscene();
		void UnloadSubscene();

		void WaitForScenes()
		{
			if(!pendingSceneLoader)
				return;

			Scene* scene = std::construct_at(static_cast<Scene*>(sceneAllocator.Allocate()), SceneConstructorKey{ this });
			SceneClass* sceneClass = pendingSceneLoader->Load(scene);

			UnloadScene();
			scenes.push_back({ scene, sceneClass });
		}

		Scene* GetScene() const { return scenes.front().scene; }
		Scene* GetSubscene(size_t i) const { return scenes.at(i + 1).scene; }

	private:
		void UnloadScene()
		{
			while(!scenes.empty())
			{
				scenes.back().sceneClass->Unload();
				scenes.back().scene->DeleteUnregisteredObject(scenes.back().sceneClass);

				std::destroy_at(scenes.back().scene);
				sceneAllocator.Free(scenes.back().scene);
				scenes.pop_back();
			}
		}
	};

	void* Object::operator new(std::size_t size, ObjectAllocator& allocator)
	{
		return allocator.Allocate(size);
	}
	void Object::operator delete(void* ptr, ObjectAllocator& allocator)
	{
		allocator.Free(ptr);
	}
	void Object::operator delete(void* ptr)
	{
		ObjectPage::GetPageFromPointer(ptr)->Free(ptr);
	}
}