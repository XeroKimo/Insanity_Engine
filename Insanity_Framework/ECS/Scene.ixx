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

export module InsanityFramework.ECS.Scene;
import InsanityFramework.Memory;
import InsanityFramework.Allocator;
import InsanityFramework.ECS.Object;

namespace InsanityFramework
{
	export class Scene;

	export class GameObject : public Object
	{
	protected:
		using Object::Object;

	};

	export class SceneSystem : public Object
	{
	protected:
		using Object::Object;
	};

	class Scene
	{
	public:
		struct Key
		{
			friend class SceneAllocator;

		private:
			Key() = default;
		};

	private:
		ObjectAllocator allocator;

		std::unordered_map<std::type_index, std::vector<GameObject*>> gameObjects;
		std::unordered_map<std::type_index, SceneSystem*> sceneSystems;
		std::vector<GameObject*> queuedConstruction;
		std::vector<GameObject*> queuedDestruction;
		std::uint32_t lifetimeLockCounter;

	public:
		Scene(Key) :
			allocator{ this }
		{

		}

		template<std::derived_from<Object> Ty, class... Args>
		Ty* NewObject(Args&&... args)
		{
			return allocator.New<Ty>(std::forward<Args>(args)...);;
		}

		template<std::derived_from<GameObject> Ty, class... Args>
		Ty* NewGameObject(Args&&... args)
		{
			Ty* object = NewObject<Ty>(std::forward<Args>(args)...);
			if(lifetimeLockCounter == 0)
				gameObjects[typeid(Ty)].push_back(object);
			else
				queuedConstruction.push_back(object);
			return object;
		}

		void DeleteObject(Object* object)
		{
			allocator.Delete(object);
		}

		void DeleteGameObject(GameObject* object)
		{
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
			if(it == sceneSystems.end())
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
		void ForEachExactType(Func func)
		{
			auto it = gameObjects.find(typeid(Ty));
			if(it == gameObjects.end())
				return;

			Ty* temp = dynamic_cast<Ty*>(it->second.front());

			auto offset = OffsetOf(it->second.front(), temp);
			for(GameObject* object : it->second)
			{
				func(*static_cast<Ty*>(IncrementPointer(object, offset)));
			}
		}

		template<class Ty, std::invocable<Ty&> Func>
		void ForEach(Func func)
		{
			for(auto& [type, objectVec] : gameObjects)
			{
				if(objectVec.size() == 0)
					continue;

				Ty* temp = dynamic_cast<Ty*>(objectVec.front());
				if(!temp)
					continue;

				auto offset = OffsetOf(objectVec.front(), temp);
				for(GameObject* object : objectVec)
				{
					func(*static_cast<Ty*>(IncrementPointer(object, offset)));
				}
			}
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

	private:
		void ImmediateDeleteGameObject(GameObject* object)
		{
			std::erase(gameObjects[typeid(*object)], object);
			DeleteObject(object);
		}
	};
}