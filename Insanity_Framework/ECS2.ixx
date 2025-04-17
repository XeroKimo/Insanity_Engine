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

	class ObjectPage
	{
		struct Block
		{
			Block* next = nullptr;
			Block* previous = nullptr;
			size_t dataBlockSize;
			bool inUse = false;

			Block* Split(size_t requestedSize)
			{
				size_t alignedSize = AlignCeil<size_t>(requestedSize + sizeof(Block), 8);

				if(alignedSize >= dataBlockSize)
					return nullptr;

				Block* newBlock = new(OffsetFromEnd(alignedSize)) Block{ .dataBlockSize = requestedSize };

				if(next)
				{
					next->previous = newBlock;
					newBlock->next = next;
				}
				newBlock->previous = this;
				next = newBlock;

				dataBlockSize -= alignedSize;

				return newBlock;
			}

			Block* SplitOrSelf(size_t requestedSize)
			{
				Block* split = Split(requestedSize);

				if(split)
					return split;

				return (requestedSize == dataBlockSize) ? this : nullptr;
			}

			bool MergeNext()
			{
				if(!next || next->inUse)
					return false;

				Block* toBeMerged = next;
				next = toBeMerged->next;
				if(next)
					next->previous = this;

				dataBlockSize += toBeMerged->BlockSize();
				std::destroy_at(toBeMerged);

				return true;
			}

			bool MergePrevious()
			{
				if(!previous || previous->inUse)
					return false;

				return previous->MergeNext();
			}

			Block* OffsetFromEnd(size_t requestedSize)
			{
				assert(requestedSize < dataBlockSize);
				return static_cast<Block*>(OffsetPointer(this, dataBlockSize - requestedSize));
			}

			size_t BlockSize() const { return dataBlockSize + sizeof(Block); }

			void* DataAddress() { return OffsetPointerAs<Block>(this, 1); }
		};

		static constexpr size_t pageSize = AlignNextPow2<size_t>(8'000'000);
		SceneManager* sceneManager;
		Scene* scene;
		ObjectPage* next;
		Block* First() { return static_cast<Block*>(OffsetPointerAs<ObjectPage>(this, 1)); }

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
			scene{ scene }
		{
			new(First()) Block{ .dataBlockSize = pageSize - sizeof(ObjectPage) - sizeof(Block) };
		}

		void* Allocate(size_t requestedSize)
		{
			Block* block = First();
			while(block->inUse)
			{
				block = block->next;
				if(!block)
					return nullptr;

				block = block->SplitOrSelf(requestedSize);
			}

			block->inUse = true;
			return block->DataAddress();
		}

		void Free(void* pointer)
		{
			Block* block = GetBlock(pointer);

			if(block->MergePrevious())
				return;

			block->inUse = false;
		}

		ObjectPage* NextPage() const
		{
			return next;
		}

		void AppendPage(ObjectPage* page)
		{
			assert(!next);

			next = page;
		}

		SceneManager* GetSceneManager() const { return sceneManager; }
		Scene* GetScene() const { return scene; }

		static ObjectPage* GetPageFromPointer(void* ptr)
		{
			return std::launder(static_cast<ObjectPage*>(AlignFloorPow2(ptr, pageSize)));
		}
	private:
		Block* GetBlock(void* pointer)
		{
			return static_cast<Block*>(OffsetPointerAs<Block>(pointer, -1));
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
				delete std::exchange(pages, pages->NextPage());
			}
		}

		void* Allocate(size_t size)
		{
			ObjectPage* currentPage = pages;
			void* ptr = currentPage->Allocate(size);
			while(!ptr)
			{
				ObjectPage* oldPage = currentPage;
				currentPage = currentPage->NextPage();
				if(!currentPage)
				{
					currentPage = new ObjectPage(oldPage->GetSceneManager(), oldPage->GetScene());
					oldPage->AppendPage(currentPage);
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
		struct Block
		{
			Block* nextFree = nullptr;
			std::optional<Scene> scene;
		};

		static constexpr size_t minimumBlocksPerPage = 16;

		static constexpr size_t PageSize()
		{
			return AlignNextPow2(sizeof(ScenePage) + sizeof(Block) * minimumBlocksPerPage);
		}

		static ScenePage* GetPageFromPointer(void* ptr)
		{
			return std::launder(static_cast<ScenePage*>(AlignFloorPow2(ptr, PageSize())));
		}

	private:
		ScenePage* next;
		SceneManager* manager;
		Block* freeList;

	public:
		ScenePage(SceneManager* manager) :
			manager{ manager }
		{
			Block* begin = BlockBegin();
			Block* end = BlockEnd();

			freeList = std::construct_at(begin);
			begin++;
			for(; begin != end; begin++)
			{
				auto oldHead = std::exchange(freeList, std::construct_at(begin));
				freeList->nextFree = oldHead;
			}
		}

		~ScenePage()
		{
			Block* begin = BlockBegin();
			Block* end = BlockEnd();

			for(; begin != end; begin++)
			{
				//Detect for leaks
				assert(!begin->scene.has_value());
			}
		}

		Scene* ConstructScene(SceneConstructorKey key) noexcept
		{
			if(!freeList)
				return nullptr;

			auto selectedBlock = freeList;
			freeList = std::exchange(selectedBlock->nextFree, nullptr);
			selectedBlock->scene = Scene{ key };

			return &selectedBlock->scene.value();
		}

		void DestroyScene(Scene* scene)
		{
			assert(GetPageFromPointer(scene) == this);

			Block* block = FindBlock(scene);
			block->scene = {};
			block->nextFree = std::exchange(freeList, block);
		}

		Block* BlockBegin()
		{
			return static_cast<Block*>(OffsetPointer(this, sizeof(ScenePage)));
		}

		Block* BlockEnd()
		{
			return BlockBegin() + (PageSize() - sizeof(ScenePage)) / sizeof(Block); 
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

		void* operator new(size_t size)
		{
			return ::operator new(PageSize(), std::align_val_t{ PageSize() });
		}

		void operator delete(void* ptr)
		{
			::operator delete(ptr, std::align_val_t{ PageSize() });
		}

	private:
		Block* FindBlock(Scene* scene)
		{
			Block* firstBlock = BlockBegin();
			std::ptrdiff_t offset = std::floor((reinterpret_cast<char*>(scene) - reinterpret_cast<char*>(firstBlock)) / sizeof(Block));
			Block* match = firstBlock + offset;

			assert(match->scene.has_value());
			assert(&match->scene.value() == scene);
			return match;
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

		Scene* Construct(SceneConstructorKey key)
		{
			ScenePage* currentPage = pages;
			Scene* scene = currentPage->ConstructScene(key);
			while(!scene)
			{
				ScenePage* oldPage = currentPage;
				currentPage = currentPage->NextPage();
				if(!currentPage)
				{
					currentPage = new ScenePage(pages->GetSceneManager());
					oldPage->AppendPage(currentPage);
				}

				scene = currentPage->ConstructScene(key);
			}

			return scene;
		}

		void Destruct(Scene* scene)
		{
			ScenePage::GetPageFromPointer(scene)->DestroyScene(scene);
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

			Scene* scene = sceneAllocator.Construct(SceneConstructorKey{ this });
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
				sceneAllocator.Destruct(scenes.back().scene);
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