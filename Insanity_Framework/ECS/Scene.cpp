module InsanityFramework.ECS.Scene;
import InsanityFramework.ECS.SceneGlobals;
import :Object;

namespace InsanityFramework
{
	Scene* Scene::GetActiveScene()
	{
		return activeScene;
	}

	Scene* Scene::GetOwner(Object* object)
	{
		return SceneGroup::GetScene(object).first;
	}

	void ObjectDeleter::operator()(Object* object)
	{
		Scene::DeleteObject(object);
	}
}