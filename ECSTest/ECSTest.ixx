module;

#include <nlohmann/json.hpp>

export module ECSTest;
import std;

export struct PrefabHandleTag {};

export template<class Ty>
struct Handle
{
	using object_type = Ty;

	union
	{
		std::uintptr_t ptr = 0;
		std::uintptr_t id;
	};

	Handle() = default;
	Handle(std::nullptr_t) {}
	Handle(Ty* ptr) : ptr{ reinterpret_cast<std::uintptr_t>(ptr) } {}

	template<class Ty2>
		requires std::is_const_v<Ty>&& std::convertible_to<Ty2, Ty>
	Handle(Handle<Ty2> handle)
	{
		ptr = handle.ptr;
	}
	Handle(std::uintptr_t handle, PrefabHandleTag) :
		id{ handle }
	{

	}
	Handle(std::uintptr_t handle);

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

	void MakeResolver();

	bool IsID() const { return (id & (1ull << 63ull)) != 0; }
	bool operator==(Ty* p) const { return p == reinterpret_cast<Ty*>(ptr); }
	bool operator==(std::nullptr_t) const { return ptr == 0; }

	Ty* get() const { return reinterpret_cast<Ty*>(ptr); }

	Ty& operator*() const { return *reinterpret_cast<Ty*>(ptr); }
	Ty* operator->() const { return reinterpret_cast<Ty*>(ptr); }
	operator bool() const { return ptr != 0; }

	template<class Ty>
	Handle<Ty> As() const
	{
		return Handle<Ty>{ dynamic_cast<Ty*>(get()) };
	}
};


export struct GameObject
{
	Handle<GameObject> parent;
	Handle<GameObject> firstChild;
	Handle<GameObject> lastChild;
	Handle<GameObject> previous;
	Handle<GameObject> next;

	std::string name;

	virtual ~GameObject() = default;

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

struct IDHandleResolver
{
	void* object;
	void(*ResolvePtr)(void*, const std::unordered_map<std::uintptr_t, GameObject*>&);

	template<class Ty>
	IDHandleResolver(Ty* object) :
		object{ object },
		ResolvePtr{ &ResolveStub<Ty> }
	{
	};

	void Resolve(const std::unordered_map<std::uintptr_t, GameObject*>& idMap)
	{
		ResolvePtr(object, idMap);
	}

	template<class Ty>
	static void ResolveStub(void* object, const std::unordered_map<std::uintptr_t, GameObject*>& idMap)
	{
		Ty* handle = static_cast<Ty*>(object);
		*handle = static_cast<Ty::object_type*>(idMap.at(handle->id));
	}
};

std::vector<IDHandleResolver> idsToResolve;

template<class Ty>
Handle<Ty>::Handle(std::uintptr_t handle) : id{ handle }
{
	MakeResolver();
}

template<class Ty>
void Handle<Ty>::MakeResolver()
{
	idsToResolve.push_back({ this });
}

std::uintptr_t g_idGenerator;


export template<std::invocable<const GameObject&> InitFunc, std::invocable<const GameObject&, int> ChildrenFunc>
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



export template<class Ty>
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

export template<class Ty>
struct Prefab;

export using GameObjectToIDMap = std::unordered_map<const GameObject*, std::uintptr_t>;

struct SerializationVTable
{
	void(*Serialize)(nlohmann::json&, const PrefabData<GameObject>&);
	std::unique_ptr<PrefabData<GameObject>>(*Deserialize)(const nlohmann::json&);
	std::unique_ptr<PrefabData<GameObject>>(*CreatePrefab)(const GameObjectToIDMap& ids, const GameObject&);
	Handle<GameObject>(*CreateGameObject)(const PrefabData<GameObject>*);
	const std::type_info&(*GetTypeInfo)();
};

export template<class Ty>
struct SerializationVTableImpl;

template<>
struct SerializationVTableImpl<GameObject>
{
	static void Serialize(nlohmann::json& json, const PrefabData<GameObject>& data)
	{

	}
	static std::unique_ptr<PrefabData<GameObject>> Deserialize(const nlohmann::json& json)
	{
		return std::make_unique<PrefabData<GameObject>>();
	}
	static std::unique_ptr<PrefabData<GameObject>> CreatePrefab(const GameObjectToIDMap& ids, const GameObject& gameObject)
	{
		return std::make_unique<PrefabData<GameObject>>();
	}
	static Handle<GameObject> CreateGameObject(const PrefabData<GameObject>* data)
	{
		return new GameObject();
	}
};

template<class Ty>
struct SerializationVTableStubs
{
	static void Serialize(nlohmann::json& json, const PrefabData<GameObject>& data)
	{
		SerializationVTableImpl<Ty>::Serialize(json, static_cast<const PrefabData<Ty>&>(data));
	}
	static std::unique_ptr<PrefabData<GameObject>> Deserialize(const nlohmann::json& json)
	{
		return SerializationVTableImpl<Ty>::Deserialize(json);
	}
	static std::unique_ptr<PrefabData<GameObject>> CreatePrefab(const GameObjectToIDMap& ids, const GameObject& gameObject)
	{
		return SerializationVTableImpl<Ty>::CreatePrefab(ids, static_cast<const Ty&>(gameObject));
	}
	static Handle<GameObject> CreateGameObject(const PrefabData<GameObject>* data)
	{
		return SerializationVTableImpl<Ty>::CreateGameObject(data ? static_cast<const PrefabData<Ty>*>(data) : data);
	}
	static const std::type_info& GetTypeInfo() { return typeid(Ty); }
};

std::unordered_map<std::uint64_t, SerializationVTable> serializeVTables;

export template<class Ty>
void RegisterGameObjectType()
{
	SerializationVTable vtable
	{
		&SerializationVTableStubs<Ty>::Serialize,
		&SerializationVTableStubs<Ty>::Deserialize,
		&SerializationVTableStubs<Ty>::CreatePrefab,
		&SerializationVTableStubs<Ty>::CreateGameObject,
		&SerializationVTableStubs<Ty>::GetTypeInfo
	};
	serializeVTables.insert(std::pair{ typeid(Ty).hash_code(), vtable });
}


export template<class Ty>
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

export Prefab<GameObject> SerializeToPrefab(const GameObject& gameObject)
{
	auto oldID = g_idGenerator;
	g_idGenerator = std::uintptr_t{ 1 } << std::uintptr_t{ 63 };

	std::queue<PrefabData<GameObject>*> childArray;
	std::unique_ptr<PrefabData<GameObject>> rootData;

	GameObjectToIDMap gameObjectToIDMap;

	ForEachGameObject(gameObject,
		[&](const GameObject& root)
	{
		gameObjectToIDMap[&root] = g_idGenerator++;
	},
		[&](const GameObject& child, int index)
	{
		gameObjectToIDMap[&child] = g_idGenerator++;
	});

	ForEachGameObject(gameObject,
		[&](const GameObject& root)
	{
		rootData = serializeVTables.at(typeid(root).hash_code()).CreatePrefab(gameObjectToIDMap, root);
		rootData->id = gameObjectToIDMap[&root];
		rootData->type = &typeid(root);
		rootData->name = root.name;

		for (auto child = root.firstChild; child; child = child->next)
		{
			childArray.push(rootData.get());
		}
	},
		[&](const GameObject& child, int index)
	{
		auto parent = childArray.front();
		childArray.pop();
		parent->children.push_back(serializeVTables.at(typeid(child).hash_code()).CreatePrefab(gameObjectToIDMap, child));

		auto childData = parent->children.back().get(); //<--- TODO: Replace this with a factory function that takes a derived game object and returns a PrefabData<Derived>
		childData->id = gameObjectToIDMap[&child];
		childData->type = &typeid(child);
		childData->name = child.name;

		for (auto child2 = child.firstChild; child2; child2 = child2->next)
		{
			childArray.push(childData);
		}
	});

	g_idGenerator = oldID;
	Prefab<GameObject> prefab;
	prefab.data = std::move(rootData);
	return prefab;
}

export std::unique_ptr<GameObject> SpawnPrefab(Prefab<GameObject> prefab)
{
	std::unordered_map<std::uintptr_t, GameObject*> idToGameObject;

	std::unique_ptr<GameObject> gameObject;

	struct UnvisitedNode
	{
		GameObject* parent;
		PrefabData<GameObject>* data;
	};

	std::queue<UnvisitedNode> unspawnedChildren;

	gameObject.reset(serializeVTables[prefab.data->type->hash_code()].CreateGameObject(prefab.data.get()).get());
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

		Handle<GameObject> childObject = serializeVTables[children.data->type->hash_code()].CreateGameObject(children.data).get();

		idToGameObject[children.data->id] = childObject.get();
		childObject->name = children.data->name;
		childObject->SetParent(children.parent);
		for (auto& child : children.data->children)
		{
			unspawnedChildren.push({ childObject.get(), child.get() });
		}
	}

	//TODO: Resolve handles to point to actual objects
	for (auto unresolvedID : idsToResolve)
	{
		unresolvedID.Resolve(idToGameObject);
	}

	idsToResolve.clear();

	return gameObject;
}

export nlohmann::json SerializePrefab(Prefab<GameObject> prefab)
{
	nlohmann::json json;

	std::queue<std::tuple<nlohmann::json*, PrefabData<GameObject>*, int>> childArray;

	json["Type"] = prefab.data->type->hash_code();
	json["ID"] = prefab.data->id; //<--- TODO: replace these serialization calls to a factory function
	json["Name"] = prefab.data->name;
	json["Children"] = nlohmann::json::array();


	serializeVTables.at(prefab.data->type->hash_code()).Serialize(json, *prefab.data);

	int i = 0;
	for (const auto& child : prefab.data->children)
	{
		json["Children"].push_back({});
		childArray.push({ &json["Children"], child.get(), i++ });
	}

	while (!childArray.empty())
	{
		auto pair = childArray.front();
		childArray.pop();
		auto& json = (*std::get<0>(pair))[std::get<2>(pair)];
		auto* child = std::get<1>(pair);

		json["Type"] = child->type->hash_code();
		json["ID"] = child->id; //<--- TODO: replace these serialization calls to a factory function
		json["Name"] = child->name;
		json["Children"] = nlohmann::json::array();

		serializeVTables.at(child->type->hash_code()).Serialize(json, *child);

		i = 0;
		for (const auto& child2 : child->children)
		{
			json["Children"].push_back({});
			childArray.push({ &json["Children"], child2.get(), i++ });
		}
	}

	return json;
}

export Prefab<GameObject> DeserializePrefab(const nlohmann::json& json)
{
	std::unique_ptr<PrefabData<GameObject>> root;

	std::queue<std::pair<PrefabData<GameObject>*, const nlohmann::json*>> children;

	root = serializeVTables[json["Type"]].Deserialize(json);
	root->type = &typeid(GameObject);
	root->id = json["ID"];
	root->name = json["Name"];

	for (auto& child : json["Children"])
	{
		children.push({ root.get(), &child });
	}

	while (!children.empty())
	{
		auto parent = children.front().first;
		auto& json = *children.front().second;
		children.pop();

		parent->children.push_back(serializeVTables[json["Type"]].Deserialize(json));
		auto child = parent->children.back().get();
		child->type = &serializeVTables[json["Type"]].GetTypeInfo();
		child->id = json["ID"];
		child->name = json["Name"];

		for (auto& childJson : json["Children"])
		{
			children.push({ child, &childJson });
		}
	}

	Prefab<GameObject> prefab;
	prefab.data = std::move(root);
	return prefab;
}