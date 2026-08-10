#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <vector>
#include <type_traits>
#include <queue>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <typeindex>

template<class Ty>
struct Handle
{
	union
	{
		std::uintptr_t ptr = 0;
		std::uintptr_t id;
	};

	Handle() = default;
	Handle(std::nullptr_t) {}
	Handle(Ty* ptr) : ptr{ reinterpret_cast<std::uintptr_t>(ptr) } {}

	template<class Ty2>
		requires std::is_const_v<Ty> && std::convertible_to<Ty2, Ty>
	Handle(Handle<Ty2> handle)
	{
		ptr = handle.ptr;
	}

	Handle& operator=(std::nullptr_t)
	{
		ptr = 0;
		return *this;
	}
	Handle& operator=(Ty* newPtr)
	{
		ptr = reinterpret_cast<std::uintptr_t>(newPtr);
		return *this;
	}
	bool operator==(Ty* p) const { return p == reinterpret_cast<Ty*>(ptr); }
	bool operator==(std::nullptr_t) const { return ptr == 0; }

	Ty* operator->() const { return reinterpret_cast<Ty*>(ptr); }
	operator bool() const { return ptr != 0; }
};

struct GameObject
{
	Handle<GameObject> parent;
	Handle<GameObject> firstChild;
	Handle<GameObject> lastChild;
	Handle<GameObject> previous;
	Handle<GameObject> next;

	std::string name;

	void SetParent(Handle<GameObject> newParent)
	{
		if (parent)
		{
			if (parent->firstChild == this)
				parent->firstChild = next;

			if (parent->lastChild == this)
				parent->lastChild = previous;

			if (previous)
				previous->next = next;

			if (next)
				next->previous = previous;

			next = nullptr;
		}

		parent = newParent;

		if (parent)
		{
			if (!parent->firstChild)
			{
				parent->firstChild = parent->lastChild = this;
				previous = nullptr;
			}
			else
			{
				parent->lastChild->next = this;
				previous = parent->lastChild;
				parent->lastChild = this;
			}
		}
		else
		{
			previous = nullptr;
		}
	}
};

std::uintptr_t g_idGenerator;


template<std::invocable<const GameObject&> Ty>
void ForEachGameObject(const GameObject& root, Ty init, Ty function)
{
	std::queue<Handle<const GameObject>> unvisitedNodes;

	init(root);
	for (auto child = root.firstChild; child; child = child->next)
		unvisitedNodes.push(child);

	while (!unvisitedNodes.empty())
	{
		auto parent = unvisitedNodes.front();
		unvisitedNodes.pop();
		function(*parent);

		for (auto child = parent->firstChild; child; child = child->next)
			unvisitedNodes.push(child);
	}
}

nlohmann::json SerializeToPrefab(const GameObject& gameObject)
{
	auto oldID = g_idGenerator;
	g_idGenerator = std::uintptr_t{ 1 } << std::uintptr_t{ 63 };
	nlohmann::json json;

	std::vector<Handle<const GameObject>> flattenTree;
	auto parent = Handle{ &gameObject };

	auto id = g_idGenerator++;

	json["ID"] = id;
	json["Name"] = parent->name;
	json["Children"] = nlohmann::json::array();

	struct UnvisitedNode
	{
		Handle<const GameObject> object;
		nlohmann::json& childArray;
	};

	std::queue<UnvisitedNode> unserializedChildren;

	unserializedChildren.push({ &gameObject, json["Children"] });

	while (!unserializedChildren.empty())
	{
		UnvisitedNode node = unserializedChildren.front();
		unserializedChildren.pop();

		for (auto child = node.object->firstChild; child; child = child->next)
		{
			node.childArray.push_back({});
			node.childArray.back()["ID"] = g_idGenerator++;
			node.childArray.back()["Name"] = child->name;
			node.childArray.back()["Children"] = nlohmann::json::array();

			unserializedChildren.push({ child, node.childArray.back()["Children"] });
		}
	}

	g_idGenerator = oldID;

	return json;
}

std::unique_ptr<GameObject> SpawnPrefab(const nlohmann::json& json)
{
	std::unordered_map<std::uintptr_t, GameObject*> idToGameObject;

	std::unique_ptr<GameObject> gameObject;

	struct UnvisitedNode
	{
		GameObject* parent;
		const nlohmann::json* childrenJson;
	};

	std::queue<UnvisitedNode> unspawnedChildren;
	auto it = json.find("ID");
	if (it != json.end())
	{
		gameObject = std::make_unique<GameObject>();
		idToGameObject[it->get<std::uintptr_t>()] = gameObject.get();
		gameObject->name = json.at("Name");
		unspawnedChildren.push({ gameObject.get(), &json.at("Children") });
	}

	while (!unspawnedChildren.empty())
	{
		auto children = unspawnedChildren.front();
		unspawnedChildren.pop();

		for (auto& childJson : *children.childrenJson)
		{
			GameObject* object = new GameObject();
			object->name = childJson.at("Name");
			idToGameObject[childJson.at("ID")] = object;
			object->SetParent(children.parent);

			unspawnedChildren.push({object, &childJson.at("Children")});
		}
	}
	return gameObject;
}

template<class Ty>
struct PrefabData;

template<>
struct PrefabData<GameObject>
{
	std::uintptr_t id;
	const std::type_info* type;

	std::string name;
	std::vector<std::unique_ptr<PrefabData<GameObject>>> children;
};

template<class Ty>
struct Prefab;

using GameObjectToIDMap = std::unordered_map<GameObject*, std::uintptr_t>;

std::unordered_map<std::uint64_t, void(*)(nlohmann::json&, const GameObjectToIDMap&, GameObject*)> serializeStub;
std::unordered_map<std::uint64_t, std::unique_ptr<PrefabData<GameObject>>(*)(const nlohmann::json&)> deserializeStub;
std::unordered_map<std::type_index, Handle<GameObject>(*)(const PrefabData<GameObject>&)> spawnStub;

template<class Ty>
struct Prefab
{
	std::shared_ptr<PrefabData<GameObject>> data;

	Prefab() = default;
	Prefab(const std::filesystem::path& path)
	{

	}

	template<std::derived_from<Ty> DTy>
	Prefab(Prefab<DTy> other) : data{ other.data }
	{

	}

	template<class Ty2>
	Prefab<Ty2> As()
	{
		Prefab<Ty2> prefab;
		prefab.data = std::dynamic_pointer_cast<Ty2>(data);
		return prefab;
	}

	const std::type_info& GetType() const { return *data->type; }
};

//Prefab<GameObject> SerializeToPrefabNew(const GameObject& gameObject)
//{
//	auto oldID = g_idGenerator;
//	g_idGenerator = std::uintptr_t{ 1 } << std::uintptr_t{ 63 };
//	nlohmann::json json;
//
//	auto parent = Handle{ &gameObject };
//
//	auto id = g_idGenerator++;
//
//	json["ID"] = id;
//	json["Name"] = parent->name;
//	json["Children"] = nlohmann::json::array();
//
//	struct UnvisitedNode
//	{
//		Handle<const GameObject> object;
//		nlohmann::json& childArray;
//	};
//
//	std::queue<Handle<const GameObject>> unserializedChildren;
//
//	unserializedChildren.push({ &gameObject, json["Children"] });
//
//	while (!unserializedChildren.empty())
//	{
//		UnvisitedNode node = unserializedChildren.front();
//		unserializedChildren.pop();
//
//		for (auto child = node.object->firstChild; child; child = child->next)
//		{
//			node.childArray.push_back({});
//			node.childArray.back()["ID"] = g_idGenerator++;
//			node.childArray.back()["Name"] = child->name;
//			node.childArray.back()["Children"] = nlohmann::json::array();
//
//			unserializedChildren.push({ child, node.childArray.back()["Children"] });
//		}
//	}
//
//	g_idGenerator = oldID;
//
//	return json;
//}

int main()
{
	GameObject a;
	a.name = "Fub";
	GameObject b;
	b.name = "Dub";
	GameObject c;
	c.name = "Cub";
	GameObject d;
	d.name = "Rub";

	b.SetParent(&a);
	c.SetParent(&b);
	d.SetParent(&a);

	nlohmann::json result = SerializeToPrefab(a);

	std::cout << "Hello world\n";

	std::cout << result << "\n";

	std::ofstream test{ "Prefab.json" };
	test << result;

	auto copy = SpawnPrefab(result);
	nlohmann::json result2 = SerializeToPrefab(*copy);
	std::cout << result2 << "\n";
	return 0;
}