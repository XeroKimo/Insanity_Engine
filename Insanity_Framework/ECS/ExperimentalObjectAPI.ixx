module;

#include <concepts>

export module InsanityFramework.ECS.ExperimentalObjectAPI;
import InsanityFramework.ECS.SceneManager;


namespace InsanityFramework::Experimental
{
	//namespace Internal
	//{
	//	export thread_local SceneManager* activeManager;
	//	export thread_local Scene* activeScene;
	//}
	//using namespace Internal;


	////Creates an object that is not registered with the scene
	////Must be paired with DeleteObject
	//export template<std::derived_from<Object> Ty, class... Args>
	//SceneUniqueObject<Ty> NewObject(Args&&... args)
	//{
	//	return activeScene->NewObject<Ty>(std::forward<Args>(args)...);
	//}

	////Creates a game object registered with the scene
	////Must be paired with DeleteGameObject
	//export template<std::derived_from<GameObject> Ty, class... Args>
	//UniqueGameObject<Ty> NewGameObject(Args&&... args)
	//{
	//	return activeScene->NewGameObject<Ty>(std::forward<Args>(args)...);
	//}

	//export void DeleteObject(Object* object)
	//{
	//	Scene::Get(object)->DeleteObject(object);
	//}

	//export void DeleteGameObject(GameObject* object)
	//{
	//	Scene::Get(object)->DeleteGameObject(object);
	//}

	//export template<std::derived_from<SceneSystem> Ty, class... Args>
	//Ty& AddSceneSystem(Args&&... args)
	//{
	//	return activeScene->AddSystem<Ty>(std::forward<Args>(args)...);
	//}

	//export template<std::derived_from<SceneSystem> Ty>
	//Ty& GetSceneSystem()
	//{
	//	return activeScene->GetSystem<Ty>();
	//}

	//export template<std::derived_from<SceneSystem> Ty>
	//Ty* TryGetSceneSystem()
	//{
	//	return activeScene->TryGetSystem<Ty>();
	//}

	//export template<std::derived_from<GameObject> Ty, std::invocable<Ty&> Func>
	//void ForEachGameObjectExactType(Func func)
	//{
	//	activeManager->GetScene()->ForEachExactType(func);
	//	for(std::size_t i = 0; i < activeManager->GetSubsceneCount(); i++)
	//	{
	//		activeManager->GetSubscene(i)->ForEachExactType(func);
	//	}
	//}

	//export template<class Ty, std::invocable<Ty&> Func>
	//void ForEachGameObject(Func func)
	//{
	//	activeManager->GetScene()->ForEachGameObject(func);
	//	for(std::size_t i = 0; i < activeManager->GetSubsceneCount(); i++)
	//	{
	//		activeManager->GetSubscene(i)->ForEachGameObject(func);
	//	}
	//}

	//export template<std::derived_from<GameObject> Ty>
	//std::vector<Ty*> GetGameObjects()
	//{
	//	auto output = activeManager->GetScene()->GetGameObjects<Ty>();
	//	for(std::size_t i = 0; i < activeManager->GetSubsceneCount(); i++)
	//	{
	//		auto temp = activeManager->GetSubscene(i)->GetGameObjects<Ty>();
	//		output.insert(output.end(), temp.begin(), temp.end());
	//	}
	//	return output;
	//}

	//export template<std::derived_from<GameObject> Ty>
	//std::vector<Ty*> GetGameObjectsOfExactType()
	//{
	//	auto output = activeManager->GetScene()->GetGameObjectsOfExactType<Ty>();
	//	for(std::size_t i = 0; i < activeManager->GetSubsceneCount(); i++)
	//	{
	//		auto temp = activeManager->GetSubscene(i)->GetGameObjectsOfExactType<Ty>();
	//		output.insert(output.end(), temp.begin(), temp.end());
	//	}
	//	return output;
	//}

	//export template<std::derived_from<SceneSystem> Ty, class... Args>
	//Ty& AddLocalSceneSystem(Object* context, Args&&... args)
	//{
	//	return Scene::Get(context)->AddSystem<Ty>(std::forward<Args>(args)...);
	//}

	//export template<std::derived_from<SceneSystem> Ty>
	//Ty& GetLocalSceneSystem(Object* context)
	//{
	//	return Scene::Get(context)->GetSystem<Ty>();
	//}

	//export template<std::derived_from<SceneSystem> Ty>
	//Ty* TryGetLocalSceneSystem(Object* context)
	//{
	//	return Scene::Get(context)->TryGetSystem<Ty>();
	//}
};