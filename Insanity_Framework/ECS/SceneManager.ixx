module;

#include <vector>
#include <optional>
#include <tuple>
#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>

export module InsanityFramework.ECS.SceneManager;
export import InsanityFramework.ECS.Scene;
import InsanityFramework.AnyRef;

namespace InsanityFramework
{
	export class SceneLoader : public Object
	{
	public:
		using Object::Object;

	public:
		virtual void Unload() = 0;
	};

	export template<class Ty, class... Args>
	concept IsSceneLoader = requires(Ty t, Args&&... args)
	{
		requires std::derived_from<Ty, SceneLoader>;
		t.Load(std::forward<Args>(args)...);
	};

	class SceneLoadPayload
	{
		struct Base
		{
			virtual ~Base() = default;
			virtual SceneUniqueObject<SceneLoader> Load(Scene* scene) = 0;
		};

		template<class Ty, class... Args>
		struct Payload : Base
		{
			std::tuple<std::remove_reference_t<Args>...> args;

			Payload(Args&&... args) :
				args{ std::forward<Args>(args)... }
			{

			}

			SceneUniqueObject<SceneLoader> Load(Scene* scene) final
			{
				auto loader = scene->NewObject<Ty>();

				if constexpr(sizeof...(Args) > 0)
				{
					std::apply([&](auto&&... args)
					{
						loader->Load(std::forward<decltype(args)>(args)...);
					}, args);
				}
				else
				{
					loader->Load();
				}

				return loader;
			}
		};

		std::unique_ptr<Base> ptr;

	public:
		template<class Ty, class... Args>
			requires IsSceneLoader<Ty, Args...>
		SceneLoadPayload(std::type_identity<Ty>, Args&&... args) :
			ptr{ std::make_unique<Payload<Ty, Args...>>(std::forward<Args>(args)...) }
		{

		}

		SceneUniqueObject<SceneLoader> Load(Scene* scene)
		{
			return ptr->Load(scene);
		}
	};

	export struct LoadedSceneResult
	{
		enum class Type
		{
			Scene,
			Subscene
		};

		Type type;
		std::size_t count = 0;
	};

	export class SceneManager
	{
	public:
		struct LoadedScenes
		{
			SceneUniquePtr scene = nullptr;
			SceneUniqueObject<SceneLoader> loader = nullptr;

			LoadedScenes(SceneUniquePtr ptr) : scene{ std::move(ptr) } {}
			LoadedScenes(const LoadedScenes&) = delete;
			LoadedScenes(LoadedScenes&&) noexcept = default;
			LoadedScenes& operator=(const LoadedScenes&) = delete;
			LoadedScenes& operator=(LoadedScenes&&) noexcept = default;
			~LoadedScenes()
			{
				if(loader)
					loader->Unload();
			}
		};

	private:
		SceneAllocator sceneAllocator{ this };
		std::vector<LoadedScenes> scenes;
		std::optional<SceneLoadPayload> pendingSceneLoad;
		std::vector<SceneLoadPayload> pendingSubscenesLoad;

	public:
		//Gets invoked when a scene wants to start loading / unloading
		std::function<void(SceneManager&, Scene&)> onSceneRequestLoad;

		//Gets invoked when a scene wants to start loading / unloading
		std::function<void(SceneManager&, Scene&)> onSceneRequestProgress;

		//Gets invoked when a scene wants to stop loading / unloading
		std::function<void(SceneManager&, Scene&)> onSceneRequestStopProgress;

		//Gets invoked when a scene wants to stop loading / unloading
		std::function<void(SceneManager&, Scene&)> onSceneLoadComplete;

		//Gets invoked when a scene wants to start loading / unloading
		std::function<void(SceneManager&, Scene&)> onSceneRequestUnload;

	public:
		template<class Ty, class... Args>
			requires IsSceneLoader<Ty, Args...>
		void LoadScene(Args&&... args)
		{
			pendingSceneLoad = SceneLoadPayload{ std::type_identity<Ty>{}, std::forward<Args>(args)... };
		}

		template<class Ty, class... Args>
			requires IsSceneLoader<Ty, Args...>
		void LoadSubscene(Args&&... args)
		{
			pendingSubscenesLoad.push_back(SceneLoadPayload{ std::type_identity<Ty>{}, std::forward<Args>(args)... });
		}

		std::optional<LoadedSceneResult> WaitForLoadingScenes()
		{
			if(pendingSceneLoad)
			{
				UnloadAllScenes();

				LoadedScenes newScene = { SceneUniquePtr{ sceneAllocator.New() } };

				if(onSceneRequestLoad)
					onSceneRequestLoad(*this, *newScene.scene);

				if(onSceneRequestProgress)
					onSceneRequestProgress(*this, *newScene.scene);

				newScene.loader = pendingSceneLoad->Load(newScene.scene.get());

				if(onSceneRequestStopProgress)
					onSceneRequestStopProgress(*this, *newScene.scene);

				if(onSceneLoadComplete)
					onSceneLoadComplete(*this, *newScene.scene);

				scenes.push_back(std::move(newScene));
				pendingSceneLoad = std::nullopt;
				return LoadedSceneResult{ LoadedSceneResult::Type::Scene, 1 };
			}
			else if(pendingSubscenesLoad.size() > 0)
			{
				for(auto& payload : pendingSubscenesLoad)
				{
					LoadedScenes newScene = { SceneUniquePtr{ sceneAllocator.New() } };

					if (onSceneRequestLoad)
						onSceneRequestLoad(*this, *newScene.scene);

					if(onSceneRequestProgress)
						onSceneRequestProgress(*this, *newScene.scene);

					newScene.loader = payload.Load(newScene.scene.get());

					if(onSceneRequestStopProgress)
						onSceneRequestStopProgress(*this, *newScene.scene);

					if (onSceneLoadComplete)
						onSceneLoadComplete(*this, *newScene.scene);

					scenes.push_back(std::move(newScene));
				}
				size_t count = pendingSubscenesLoad.size();
				pendingSubscenesLoad.clear();
				return LoadedSceneResult{ LoadedSceneResult::Type::Subscene, count };
			}

			return std::nullopt;
		}

		bool HasAnySceneLoading() const noexcept { return pendingSceneLoad || !pendingSubscenesLoad.empty(); }

		Scene* GetScene() { return scenes.front().scene.get(); }
		const Scene* GetScene() const { return scenes.front().scene.get(); }

		Scene* GetSubscene(std::size_t i) { return scenes[i - 1].scene.get(); }
		const Scene* GetSubscene(std::size_t i) const { return scenes[i - 1].scene.get(); }

		std::size_t GetSubsceneCount() const { return scenes.size() - 1; }

	private:
		void UnloadAllScenes()
		{
			while(!scenes.empty())
			{
				auto oldScene = std::move(scenes.back());
				scenes.pop_back();

				if (onSceneRequestUnload)
					onSceneRequestUnload(*this, *oldScene.scene);

				if(onSceneRequestProgress)
					onSceneRequestProgress(*this, *oldScene.scene);

				if (onSceneRequestStopProgress)
					onSceneRequestStopProgress(*this, *oldScene.scene);
			}
		}
	};

	export class SceneManagerLifetimeScopeLock
	{
	private:
		SceneManager& manager;

	public:
		SceneManagerLifetimeScopeLock(SceneManager& manager) :
			manager{ manager }
		{
			manager.GetScene()->LockLifetimes();
			for(size_t i = 0; i < manager.GetSubsceneCount(); i++)
			{
				manager.GetSubscene(i)->LockLifetimes();
			}
		}

		~SceneManagerLifetimeScopeLock()
		{
			for(size_t i = 0; i < manager.GetSubsceneCount(); i++)
			{
				manager.GetSubscene(i)->UnlockLifetimes();
			}
			manager.GetScene()->UnlockLifetimes();
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