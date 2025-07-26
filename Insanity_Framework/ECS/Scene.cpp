module InsanityFramework.ECS.Scene;
import InsanityFramework.ECS.SceneGlobals;

namespace InsanityFramework
{
	Scene* Scene::GetActiveScene()
	{
		return activeScene;
	}
}