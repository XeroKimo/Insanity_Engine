module;

#include <typeindex>
#include <filesystem>
#include <functional>
#include <any>
#include <unordered_map>

export module InsanityFramework.AssetLoader;

namespace InsanityFramework
{
	void InternalRegisterAssetLoader(std::type_index type, std::function<std::any(std::filesystem::path)> loaderFunction);
	void InternalUnregisterAssetLoader(std::type_index type);
	std::any InternalLoadAsset(std::type_index type, std::filesystem::path path);

	export template<class Ty, class Func>
		requires std::is_invocable_r_v<Ty, Func, std::filesystem::path>
	void RegisterAssetLoader(Func&& loaderFunction)
	{
		InternalRegisterAssetLoader(typeid(Ty), std::forward<Func>(loaderFunction));
	}

	export template<class Ty>
	void UnregisterAssetLoader()
	{
		InternalUnregisterAssetLoader(typeid(Ty));
	}

	export template<class Ty>
	Ty LoadAsset(std::filesystem::path path)
	{
		return std::any_cast<Ty>(InternalLoadAsset(typeid(Ty), path));
	}
}

module:private;

namespace InsanityFramework
{
	std::unordered_map<std::type_index, std::function<std::any(std::filesystem::path)>> loaderFunctions;

	void InternalRegisterAssetLoader(std::type_index type, std::function<std::any(std::filesystem::path)> loaderFunction)
	{
		loaderFunctions.insert({ type, loaderFunction });
	}
	void InternalUnregisterAssetLoader(std::type_index type)
	{
		loaderFunctions.erase(type);
	}
	std::any InternalLoadAsset(std::type_index type, std::filesystem::path path)
	{
		return loaderFunctions.at(type)(path);
	}
}