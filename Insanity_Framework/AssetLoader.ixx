module;

#include <typeindex>
#include <filesystem>
#include <functional>
#include <any>
#include <unordered_map>

export module InsanityFramework.AssetLoader;

namespace InsanityFramework
{
	namespace AssetLoader
	{
		void InternalRegister(std::type_index type, std::function<std::any(std::filesystem::path)> loaderFunction);
		void InternalUnregister(std::type_index type);
		std::any InternalLoad(std::type_index type, std::filesystem::path path);

		export template<class Ty, class Func>
			requires std::is_invocable_r_v<Ty, Func, std::filesystem::path>
		void Register(Func&& loaderFunction)
		{
			InternalRegister(typeid(Ty), std::forward<Func>(loaderFunction));
		}

		export template<class Ty>
		void Unregister()
		{
			InternalUnregister(typeid(Ty));
		}

		export template<class Ty>
		Ty Load(std::filesystem::path path)
		{
			return std::any_cast<Ty>(InternalLoad(typeid(Ty), path));
		}
	}
}

module:private;

namespace InsanityFramework
{
	namespace AssetLoader
	{
		std::unordered_map<std::type_index, std::function<std::any(std::filesystem::path)>> loaderFunctions;

		void InternalRegister(std::type_index type, std::function<std::any(std::filesystem::path)> loaderFunction)
		{
			loaderFunctions.insert({ type, loaderFunction });
		}
		void InternalUnregister(std::type_index type)
		{
			loaderFunctions.erase(type);
		}
		std::any InternalLoad(std::type_index type, std::filesystem::path path)
		{
			return loaderFunctions.at(type)(path);
		}
	}
}