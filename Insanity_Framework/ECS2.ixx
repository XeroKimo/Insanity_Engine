//module;
//
//#include <optional>
//#include <new>
//#include <cassert>
//#include <cmath>
//#include <vector>
//#include <unordered_map>
//#include <memory>
//
//export module InsanityFramework.ECS2;
//import InsanityFramework.Memory;
//import InsanityFramework.Allocator;
//
//namespace InsanityFramework
//{
//	template<class Ty>
//	struct Identity {};
//
//	class SceneClass : public Object
//	{
//
//	public:
//		using Object::Object;
//
//		virtual void Unload() = 0;
//	};
//
//	template<class Ty, class... Args>
//	concept IsSceneClass = requires(Ty t, Args&&... args)
//	{
//		requires std::derived_from<Ty, SceneClass>;
//		t.Load(std::forward<Args>(args)...);
//	};
//
//	class SceneClassFactory
//	{
//		struct Base
//		{
//			virtual ~Base() = default;
//
//			virtual SceneClass* Load(Scene* scene) = 0;
//		};
//
//		template<IsSceneClass Ty, class... Args>
//		struct Derived
//		{
//			std::tuple<Args...> args;
//			Derived(Args&&... data) : args{ std::forward<Args>(data)... }
//			{
//
//			}
//
//			SceneClass* Load(Scene* scene)
//			{
//				Ty* sceneClass = scene->NewUnregisteredObject<Ty>();
//				if constexpr(sizeof...(Args) > 0)
//				{
//					std::apply([&sceneClass](Args&&... params)
//					{
//						sceneClass->Load(std::forward<Args>(params)...);
//					}, args);
//				}
//				else
//				{
//					sceneClass->Load();
//				}
//				return sceneClass;
//			}
//		};
//
//		std::unique_ptr<Base> ptr;
//
//	public:
//		template<IsSceneClass Ty, class... Args>
//		SceneClassFactory(std::type_identity<Ty>, Args&&... params) :
//			ptr{ std::make_unique<Derived<Ty, Args...>>(std::forward<Args>(params)...) }
//		{
//
//		}
//
//		SceneClass* Load(Scene* scene)
//		{
//			return ptr->Load(scene);
//		}
//	};
//
//	class SceneManager
//	{
//		struct LoadedScenes
//		{
//			Scene* scene;
//			SceneClass* sceneClass;
//		};
//
//	private:
//		SceneAllocator sceneAllocator;
//		std::vector<LoadedScenes> scenes;
//		std::optional<SceneClassFactory> pendingSceneLoader;
//
//	public:
//
//		~SceneManager()
//		{
//			UnloadScene();
//		}
//
//
//		void LoadScene();
//		void LoadSubscene();
//		void UnloadSubscene();
//
//		void WaitForScenes()
//		{
//			if(!pendingSceneLoader)
//				return;
//
//			Scene* scene = std::construct_at(static_cast<Scene*>(sceneAllocator.Allocate()), SceneConstructorKey{ this });
//			SceneClass* sceneClass = pendingSceneLoader->Load(scene);
//
//			UnloadScene();
//			scenes.push_back({ scene, sceneClass });
//		}
//
//		Scene* GetScene() const { return scenes.front().scene; }
//		Scene* GetSubscene(size_t i) const { return scenes.at(i + 1).scene; }
//
//	private:
//		void UnloadScene()
//		{
//			while(!scenes.empty())
//			{
//				scenes.back().sceneClass->Unload();
//				scenes.back().scene->DeleteUnregisteredObject(scenes.back().sceneClass);
//
//				std::destroy_at(scenes.back().scene);
//				sceneAllocator.Free(scenes.back().scene);
//				scenes.pop_back();
//			}
//		}
//	};
//}