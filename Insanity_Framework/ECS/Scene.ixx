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
export import :Object;
export import InsanityFramework.TransformationNode;

namespace InsanityFramework
{
	export class Scene;

	export class GameObject : public Object, public TransformNode
	{
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
	};

	export class SceneSystem
	{
	public:
		virtual ~SceneSystem() = default;
	};

	export struct SceneCallbacks
	{
		virtual void OnObjectCreated(Object* object) {}
		virtual void OnObjectDestroyed(Object* object) {}
	};
	export SceneCallbacks defaultSceneCallbacks;

	export class SceneGroup;

	template<class Ty>
	class ExactObjectIterator
	{
		using iterator_concept = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;

		std::vector<Object*>::const_iterator iterator{};
		std::ptrdiff_t offset{};

	public:
		ExactObjectIterator() = default;
		ExactObjectIterator(std::vector<Object*>::const_iterator iteartor, std::ptrdiff_t offset) :
			iterator{ iteartor },
			offset{ offset }
		{

		}

	public:
		pointer operator*() const noexcept {
			return std::launder(static_cast<pointer>(IncrementPointer(*iterator, offset)));
		}

		pointer operator->() const noexcept {
			return std::launder(static_cast<pointer>(IncrementPointer(*iterator, offset)));
		}

		ExactObjectIterator& operator++() noexcept {
			++iterator;
			return *this;
		}

		ExactObjectIterator operator++(int) noexcept {
			ExactObjectIterator temp = *this;
			++*this;
			return temp;
		}

		ExactObjectIterator& operator--() noexcept {
			--iterator;
			return *this;
		}

		ExactObjectIterator operator--(int) noexcept {
			ExactObjectIterator temp = *this;
			--*this;
			return temp;
		}

		ExactObjectIterator& operator+=(const difference_type offset) noexcept {
			iterator += offset;
			return *this;
		}

		ExactObjectIterator operator+(const difference_type offset) const noexcept {
			ExactObjectIterator temp = *this;
			temp += offset;
			return temp;
		}

		friend ExactObjectIterator operator+(const difference_type offset, ExactObjectIterator iterator) noexcept {
			iterator += offset;
			return iterator;
		}

		ExactObjectIterator& operator-=(const difference_type offset) noexcept {
			return *this += -offset;
		}

		ExactObjectIterator operator-(const difference_type offset) const noexcept {
			ExactObjectIterator temp = *this;
			temp -= offset;
			return temp;
		}

		difference_type operator-(const ExactObjectIterator& right) const noexcept {
			return static_cast<difference_type>(iterator - right.iterator);
		}

		reference operator[](const difference_type offset) const noexcept {
			return *std::launder(static_cast<pointer>(IncrementPointer(iterator[offset], this->offset)));
		}

		bool operator==(const ExactObjectIterator& right) const noexcept {
			return iterator == right.iterator;
		}


		constexpr std::strong_ordering operator<=>(const ExactObjectIterator& right) const noexcept {
			return iterator <=> right.iterator;
		}
	};

	template<class Ty>
	class ExactObjectRange
	{
		const std::vector<Object*>* objects;
		std::ptrdiff_t offset;

	public:
		ExactObjectRange(const Scene* scene);

		ExactObjectIterator<Ty> begin() const
		{
			return objects ? ExactObjectIterator<Ty>{ objects->begin(), offset } : ExactObjectIterator<Ty>{};
		}

		ExactObjectIterator<Ty> end() const
		{
			return objects ? ExactObjectIterator<Ty>{ objects->end(), offset } : ExactObjectIterator<Ty>{};
		}
	};	
	
	template<class Ty>
	class ObjectSentinal
	{
		template<class Ty>
		friend class ObjectIterator;

		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;
	};

	template<class Ty>
	class ObjectIterator
	{
		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;

		std::unordered_map<std::type_index, std::vector<Object*>>::const_iterator currentMapIt{};
		std::unordered_map<std::type_index, std::vector<Object*>>::const_iterator endMapIt{};
		std::vector<Object*>::const_iterator currentIt{};
		std::vector<Object*>::const_iterator endIt{};
		std::ptrdiff_t offset{};

	public:
		ObjectIterator() = default;
		ObjectIterator(const Scene* scene);

	public:
		pointer operator*() const noexcept {
			return std::launder(static_cast<pointer>(IncrementPointer(*currentIt, offset)));
		}

		pointer operator->() const noexcept {
			return std::launder(static_cast<pointer>(IncrementPointer(*currentIt, offset)));
		}

		ObjectIterator& operator++() noexcept {

			currentIt++;

			if (currentIt == endIt)
			{
				currentMapIt++;
				for (; currentMapIt != endMapIt; currentMapIt++)
				{
					currentIt = currentMapIt->second.begin();
					auto temp = dynamic_cast<Ty*>(*currentIt);
					if (!temp)
						continue;

					offset = OffsetOf(*currentIt, temp);
					endIt = currentMapIt->second.end();

					break;
				}
			}
			return *this;
		}

		ObjectIterator operator++(int) noexcept {
			ObjectIterator temp = *this;
			++*this;
			return temp;
		}

		bool operator==(const ObjectSentinal<Ty>& right) const noexcept {
			return currentMapIt == endMapIt;
		}
	};

	template<class Ty>
	class ObjectRange
	{
		const Scene* scene;

	public:
		ObjectRange(const Scene* scene) :
			scene{ scene }
		{

		}

		ObjectIterator<Ty> begin() const
		{
			return { scene };
		}

		ObjectSentinal<Ty> end() const
		{
			return { };
		}
	};
	

	template<class Ty>
	class GlobalObjectSentinal
	{
		template<class Ty>
		friend class GlobalObjectIterator;

		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;
	};

	template<class Ty>
	class GlobalObjectIterator
	{
		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;

		std::span<const std::unique_ptr<Scene>>::iterator sceneItCurrent{};
		std::span<const std::unique_ptr<Scene>>::iterator sceneItEnd{};
		ObjectIterator<Ty> currentIt;
		ObjectSentinal<Ty> endIt;

	public:
		GlobalObjectIterator() = default;
		GlobalObjectIterator(const SceneGroup* sceneGroup);

	public:
		pointer operator*() const noexcept {
			return currentIt.operator*();
		}

		pointer operator->() const noexcept {
			return currentIt.operator->();
		}

		GlobalObjectIterator& operator++() noexcept {

			currentIt++;

			if (currentIt == endIt)
			{
				sceneItCurrent++;
				for (; sceneItCurrent != sceneItEnd; sceneItCurrent++)
				{
					ObjectRange<Ty> range{ sceneItCurrent->get() };
					currentIt = range.begin();
					endIt = range.end();

					if (currentIt != endIt)
						break;
				}
			}
			return *this;
		}

		GlobalObjectIterator operator++(int) noexcept {
			ObjectIterator temp = *this;
			++*this;
			return temp;
		}

		bool operator==(const GlobalObjectSentinal<Ty>& right) const noexcept {
			return sceneItCurrent == sceneItEnd;
		}
	};

	template<class Ty>
	class GlobalObjectRange
	{
		const SceneGroup* scene;

	public:
		GlobalObjectRange(const SceneGroup* scene) :
			scene{ scene }
		{

		}

		GlobalObjectIterator<Ty> begin() const
		{
			return { scene };
		}

		GlobalObjectSentinal<Ty> end() const
		{
			return { };
		}
	};



	template<class Ty>
	class GlobalExactObjectSentinal
	{
		template<class Ty>
		friend class GlobalExactObjectIterator;

		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;
	};

	template<class Ty>
	class GlobalExactObjectIterator
	{
		template<class Ty>
		friend class GlobalExactObjectSentinal;

		using iterator_concept = std::forward_iterator_tag;
		using value_type = Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;

		std::span<const std::unique_ptr<Scene>>::iterator sceneItCurrent{};
		std::span<const std::unique_ptr<Scene>>::iterator sceneItEnd{};
		ExactObjectIterator<Ty> current;
		ExactObjectIterator<Ty> end;


	public:
		GlobalExactObjectIterator() = default;
		GlobalExactObjectIterator(const SceneGroup* sceneGroup);

	public:
		pointer operator*() const noexcept {
			return *current;
		}

		pointer operator->() const noexcept {
			return current.operator->();
		}

		GlobalExactObjectIterator& operator++() noexcept {
			current++;
			if (current == end)
			{
				sceneItCurrent++;
				while (sceneItCurrent != sceneItEnd)
				{
					ExactObjectRange<Ty> range{ sceneItCurrent->get() };
					current = range.begin();
					end = range.end();

					if (current != end)
						break;

					sceneItCurrent++;
				}
			}
			return *this;
		}

		GlobalExactObjectIterator operator++(int) noexcept {
			GlobalExactObjectIterator temp = *this;
			++*this;
			return temp;
		}

		bool operator==(const GlobalExactObjectSentinal<Ty>& right) const noexcept {
			return sceneItCurrent == sceneItEnd;
		}
	};

	template<class Ty>
	class GlobalExactObjectRange
	{
		const SceneGroup* group;

	public:
		GlobalExactObjectRange(const SceneGroup* group) :
			group{ group }
		{

		}

		GlobalExactObjectIterator<Ty> begin() const
		{
			return { group };
		}

		GlobalExactObjectSentinal<Ty> end() const
		{
			return { };
		}
	};

	class Scene
	{
		template<class Ty>
		friend class ExactObjectRange;

		template<class Ty>
		friend class GlobalExactObjectIterator;

		template<class Ty>
		friend class ObjectIterator;
	public:
		struct Key
		{
			friend class SceneGroup;
		private:
			Key() = default;
		};

	private:
		ObjectAllocator allocator;

		std::unordered_map<std::type_index, std::vector<Object*>> gameObjects;
		std::unordered_map<std::type_index, std::unique_ptr<SceneSystem>> sceneSystems;
		std::vector<Object*> queuedConstruction;
		std::vector<Object*> queuedDestruction;
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
			std::vector<Object*> rootObjects;
			for(auto& [type, objects] : gameObjects)
			{
				for(Object* object : objects)
				{
					if(object->IsRoot())
						rootObjects.push_back(object);
				}
			}

			for(Object* object : rootObjects)
			{
				DeleteObject(object);
				//allocator.Delete(object);
			}

			for(auto& [type, object] : sceneSystems)
			{
				object = nullptr;
			}
		}

		bool Contains(Object* object) const
		{
			return allocator.Contains(object);
		}

		//Creates a game object registered with the scene
		template<std::derived_from<GameObject> Ty, class... Args>
		static UniqueObject<Ty> NewObject(Args&&... args)
		{
			UniqueObject<Ty> object = GetActiveScene()->allocator.New<Ty>(std::forward<Args>(args)...);
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

			if(owner->lifetimeLockCounter == 0)
			{
				owner->ImmediateDeleteObject(object);
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
			auto system = std::make_unique<Ty>(std::forward<Args>(args)...);
			auto output = system.get();
			GetActiveScene()->sceneSystems.insert({ typeid(Ty),  std::move(system) });
			return *output;
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


		template<std::derived_from<GameObject> Ty>
		static GlobalExactObjectRange<Ty> GetObjectsExactType() 
		{
			return { SceneGroup::GetGroup(GetActiveScene()) };
		}

		template<std::derived_from<GameObject> Ty>
		static ExactObjectRange<Ty> GetObjectsExactTypeInScene(Scene* scene) 
		{
			return { scene };
		}

		template<class Ty>
		static GlobalObjectRange<Ty> GetObjects() 
		{
			return { SceneGroup::GetGroup(GetActiveScene()) };
		}

		template<class Ty>
		static ObjectRange<Ty> GetObjectsInScene(Scene* scene) 
		{
			return { scene };
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

			for(Object* object : queuedConstruction)
			{
				gameObjects[typeid(*object)].push_back(object);
			}
			queuedConstruction.clear();

			for(Object* object : queuedDestruction)
			{
				ImmediateDeleteObject(object);
			}
			queuedDestruction.clear();
		}

	private:
		void ImmediateDeleteObject(Object* object)
		{
			callbacks->OnObjectDestroyed(object);
			std::erase(gameObjects[typeid(*object)], object);
			allocator.Delete(object);
		}

		static Scene* GetOwner(Object* object);
	};

	struct SceneGroupDeleter
	{
		void operator()(SceneGroup* group);
	};

	struct SceneHandleDeleter
	{
		void operator()(Scene* scene);
	};

	export class SceneGroup;
	export using UniqueSceneHandle = std::unique_ptr<Scene, SceneHandleDeleter>;
	export using UniqueSceneGroupHandle = std::unique_ptr<SceneGroup, SceneGroupDeleter>;

	export class SceneGroup
	{
		inline static std::list<SceneGroup> groups;

		std::vector<std::unique_ptr<Scene>> scenes;

	private:
		SceneGroup() = default;

	public:
		static UniqueSceneGroupHandle New()
		{
			groups.push_back({});

			return { &groups.back(), {} };
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

		static void Delete(SceneGroup* group)
		{
			std::erase_if(groups, [=](const auto& g) { return &g == group; });
		}

		static std::pair<Scene*, SceneGroup*> GetScene(Object* object)
		{
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

		std::span<const std::unique_ptr<Scene>> GetScenes() const { return scenes; }
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

	void SceneGroupDeleter::operator()(SceneGroup* group)
	{
		SceneGroup::Delete(group);
	}

	template<class Ty>
	ExactObjectRange<Ty>::ExactObjectRange(const Scene* scene) :
		objects{ scene->gameObjects.contains(typeid(Ty)) ? &scene->gameObjects.at(typeid(Ty)) : nullptr },
		offset{ objects && !objects->empty() ? OffsetOf(objects->front(), dynamic_cast<Ty*>(objects->front())) : 0  }
	{
	}

	template<class Ty>
	GlobalExactObjectIterator<Ty>::GlobalExactObjectIterator(const SceneGroup* sceneGroup) :
		sceneItCurrent{ sceneGroup->GetScenes().begin() },
		sceneItEnd{ sceneGroup->GetScenes().end() }
	{
		while (sceneItCurrent != sceneItEnd)
		{
			ExactObjectRange<Ty> range{ sceneItCurrent->get() };
			current = range.begin();
			end = range.end();

			if (current != end)
				break;

			sceneItCurrent++;
		}
	}

	template<class Ty>
	ObjectIterator<Ty>::ObjectIterator(const Scene* scene) :
		currentMapIt{ scene->gameObjects.begin() },
		endMapIt{ scene->gameObjects.end() }
	{
		for (; currentMapIt != endMapIt; currentMapIt++)
		{
			currentIt = currentMapIt->second.begin();
			auto temp = dynamic_cast<Ty*>(*currentIt);
			if (!temp)
				continue;
			
			offset = OffsetOf(*currentIt, temp);
			endIt = currentMapIt->second.end();

			break;
		}
	}
	template<class Ty>
	GlobalObjectIterator<Ty>::GlobalObjectIterator(const SceneGroup* sceneGroup) :
		sceneItCurrent{ sceneGroup->GetScenes().begin() },
		sceneItEnd{ sceneGroup->GetScenes().end() }
	{
		for (; sceneItCurrent != sceneItEnd; sceneItCurrent++)
		{
			ObjectRange<Ty> range{ sceneItCurrent->get() };
			currentIt = range.begin();
			endIt = range.end();

			if (currentIt != endIt)
				break;
		}
	}
}