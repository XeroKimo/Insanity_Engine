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

	Ty& operator*() const { return *reinterpret_cast<Ty*>(ptr); }
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


template<std::invocable<const GameObject&> InitFunc, std::invocable<const GameObject&, int> ChildrenFunc>
void ForEachGameObject(const GameObject& root, InitFunc init, ChildrenFunc function)
{
	std::queue<std::pair<Handle<const GameObject>, int>> unvisitedNodes;

	init(root);
	int i = 0;
	for (auto child = root.firstChild; child; child = child->next, i++)
		unvisitedNodes.push({ child, i });

	while (!unvisitedNodes.empty())
	{
		auto parent = unvisitedNodes.front();
		unvisitedNodes.pop();
		function(*parent.first, parent.second);

		i = 0;
		for (auto child = parent.first->firstChild; child; child = child->next, i++)
			unvisitedNodes.push({ child, i });
	}
}

nlohmann::json SerializeToPrefab(const GameObject& gameObject)
{
	auto oldID = g_idGenerator;
	g_idGenerator = std::uintptr_t{ 1 } << std::uintptr_t{ 63 };
	nlohmann::json json;

	std::queue<nlohmann::json*> childArray;

	ForEachGameObject(gameObject,
		[&](const GameObject& root)
	{
		json["ID"] = g_idGenerator++;
		json["Name"] = root.name;
		json["Children"] = nlohmann::json::array();

		for (auto child = root.firstChild; child; child = child->next)
		{
			json["Children"].push_back({});
			childArray.push(&json["Children"]);
		}
	},
	[&](const GameObject& child, int index)
	{
		auto& json = (*childArray.front())[index];
		childArray.pop();
		json["ID"] = g_idGenerator++;
		json["Name"] = child.name;
		json["Children"] = nlohmann::json::array();

		for (auto child2 = child.firstChild; child2; child2 = child2->next)
		{
			json["Children"].push_back({});
			childArray.push(&json["Children"]);
		}
	});

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

	virtual ~PrefabData() = default;
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

Prefab<GameObject> SerializeToPrefabNew(const GameObject& gameObject)
{
	auto oldID = g_idGenerator;
	g_idGenerator = std::uintptr_t{ 1 } << std::uintptr_t{ 63 };

	std::queue<PrefabData<GameObject>*> childArray;
	std::unique_ptr<PrefabData<GameObject>> rootData;

	ForEachGameObject(gameObject,
		[&](const GameObject& root)
	{
		rootData = std::make_unique<PrefabData<GameObject>>();
		rootData->id = g_idGenerator++;
		rootData->type = &typeid(root);
		rootData->name = root.name;

		for (auto child = root.firstChild; child; child = child->next)
		{
			rootData->children.push_back(std::make_unique<PrefabData<GameObject>>());
			childArray.push(rootData->children.back().get());
		}
	},
		[&](const GameObject& child, int index)
	{
		auto childData = childArray.front();
		childArray.pop();
		childData->id = g_idGenerator++;
		childData->type = &typeid(child);
		childData->name = child.name;

		for (auto child2 = child.firstChild; child2; child2 = child2->next)
		{
			childData->children.push_back(std::make_unique<PrefabData<GameObject>>());
			childArray.push(childData->children.back().get());
		}
	});

	g_idGenerator = oldID;
	Prefab<GameObject> prefab;
	prefab.data = std::move(rootData);
	return prefab;
}

std::unique_ptr<GameObject> SpawnPrefab(Prefab<GameObject> prefab)
{
	std::unordered_map<std::uintptr_t, GameObject*> idToGameObject;

	std::unique_ptr<GameObject> gameObject;

	struct UnvisitedNode
	{
		GameObject* parent;
		PrefabData<GameObject>* data;
	};

	std::queue<UnvisitedNode> unspawnedChildren;

	gameObject = std::make_unique<GameObject>(); //<-- replace this with a factory function
	idToGameObject[prefab.data->id] = gameObject.get();
	gameObject->name = prefab.data->name;

	for (auto& child : prefab.data->children)
	{
		unspawnedChildren.push({ gameObject.get(), child.get() });
	}

	while (!unspawnedChildren.empty())
	{
		auto children = unspawnedChildren.front();
		unspawnedChildren.pop();

		GameObject* childObject = new GameObject(); //<--- replace this with a factory function

		idToGameObject[prefab.data->id] = childObject;
		childObject->name = children.data->name;
		childObject->SetParent(children.parent);
		for (auto& child : children.data->children)
		{
			unspawnedChildren.push({ childObject, child.get() });
		}
	}
	return gameObject;
}

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
	auto prefab = SerializeToPrefabNew(a);
	std::cout << "Hello world\n";

	std::cout << result << "\n";

	std::ofstream test{ "Prefab.json" };
	test << result;

	auto copy = SpawnPrefab(result);
	auto copy2 = SpawnPrefab(prefab);
	nlohmann::json result2 = SerializeToPrefab(*copy);
	std::cout << result2 << "\n";

	nlohmann::json result3 = SerializeToPrefab(*copy2);
	std::cout << result3 << "\n";
	return 0;
}