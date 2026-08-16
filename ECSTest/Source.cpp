#include <nlohmann/json.hpp>

import ECSTest;
import std;



struct B : GameObject
{
	float color;
};

struct C : B
{
	bool something;
};

struct D : GameObject
{
	Handle<B> someB;
};

template<>
struct PrefabData<B> : public PrefabData<GameObject>
{
	float color;
};

template<>
struct SerializationVTableImpl<B>
{
	static void Serialize(nlohmann::json& json, const PrefabData<B>& data)
	{
		json["Color"] = data.color;
	}
	static std::unique_ptr<PrefabData<GameObject>> Deserialize(const nlohmann::json& json)
	{
		std::unique_ptr<PrefabData<B>> data = std::make_unique<PrefabData<B>>();
		data->color = json.at("Color");

		return data;
	}
	static std::unique_ptr<PrefabData<GameObject>> CreatePrefab(const GameObjectToIDMap& ids, const B& gameObject)
	{
		std::unique_ptr<PrefabData<B>> data = std::make_unique<PrefabData<B>>();
		data->color = gameObject.color;
		return data;
	}
	static Handle<GameObject> CreateGameObject(const PrefabData<GameObject>* data)
	{
		B* gameObject = new B();

		if (auto bData = dynamic_cast<const PrefabData<B>*>(data); bData)
		{
			gameObject->color = bData->color;
		}

		return gameObject;
	}
};

template<>
struct PrefabData<C> : public PrefabData<B>
{
	bool something;
};

template<>
struct SerializationVTableImpl<C>
{
	static void Serialize(nlohmann::json& json, const PrefabData<C>& data)
	{
		SerializationVTableImpl<B>::Serialize(json, data);
		json["Something"] = data.something;
	}
	static std::unique_ptr<PrefabData<GameObject>> Deserialize(const nlohmann::json& json)
	{
		std::unique_ptr<PrefabData<C>> data = std::make_unique<PrefabData<C>>();
		data->color = json["Color"];
		data->something = json["Something"];

		return data;
	}
	static std::unique_ptr<PrefabData<GameObject>> CreatePrefab(const GameObjectToIDMap& ids, const C& gameObject)
	{
		std::unique_ptr<PrefabData<C>> data = std::make_unique<PrefabData<C>>();
		data->color = gameObject.color;
		data->something = gameObject.something;
		return data;
	}
	static Handle<GameObject> CreateGameObject(const PrefabData<GameObject>* data)
	{
		C* gameObject = new C();

		if (auto bData = dynamic_cast<const PrefabData<C>*>(data); bData)
		{
			gameObject->color = bData->color;
			gameObject->something = bData->something;
		}

		return gameObject;
	}
};

template<>
struct PrefabData<D> : public PrefabData<GameObject>
{
	Handle<B> someB;
};

template<>
struct SerializationVTableImpl<D>
{
	static void Serialize(nlohmann::json& json, const PrefabData<D>& data)
	{
		json["SomeB"] = data.someB.id;
	}
	static std::unique_ptr<PrefabData<GameObject>> Deserialize(const nlohmann::json& json)
	{
		std::unique_ptr<PrefabData<D>> data = std::make_unique<PrefabData<D>>();
		data->someB = Handle<B>{ json["SomeB"].get<std::uintptr_t>(), PrefabHandleTag{} };

		return data;
	}
	static std::unique_ptr<PrefabData<GameObject>> CreatePrefab(const GameObjectToIDMap& ids, const D& gameObject)
	{
		std::unique_ptr<PrefabData<D>> data = std::make_unique<PrefabData<D>>();
		data->someB = Handle<B>{ ids.at(gameObject.someB.get()), PrefabHandleTag{} };
		return data;
	}
	static Handle<GameObject> CreateGameObject(const PrefabData<GameObject>* data)
	{
		D* gameObject = new D();

		if (auto bData = dynamic_cast<const PrefabData<D>*>(data); bData)
		{
			gameObject->someB = bData->someB;
			gameObject->someB.MakeResolver();
		}

		return gameObject;
	}
};


int main()
{

	RegisterGameObjectType<GameObject>();
	RegisterGameObjectType<B>();
	RegisterGameObjectType<C>();
	RegisterGameObjectType<D>();

	GameObject a;
	B b;
	C c;
	D d;

	a.name = "Fub";
	b.name = "Dub";
	c.name = "Cub";
	d.name = "Rub";
	b.SetParent(&a);
	c.SetParent(&b);
	d.SetParent(&a);

	b.color = 1.0f;
	c.color = 0.32f;
	c.something = true;
	d.someB = &b;

	Prefab<GameObject> prefab = SerializeToPrefab(a);
	auto clone = SpawnPrefab(prefab);

	std::queue<Handle<GameObject>> queue;
	queue.push(clone.get());

	while (!queue.empty())
	{
		auto top = queue.front();
		queue.pop();
		std::println("{}", typeid(*top).name());
		for (auto child = top->firstChild; child; child = child->next)
		{
			queue.push(child);
		}
	}

	DeserializePrefab(SerializePrefab(prefab));

	//RegisterGameObjectType<GameObject>();

	//GameObject a;
	//a.name = "Fub";
	//GameObject b;
	//b.name = "Dub";
	//GameObject c;
	//c.name = "Cub";
	//GameObject d;
	//d.name = "Rub";

	//b.SetParent(&a);
	//c.SetParent(&b);
	//d.SetParent(&a);

	//auto prefab = SerializeToPrefab(a);
	//std::cout << "Hello world\n";

	//std::ofstream test{ "Prefab.json" };

	//auto copy = SpawnPrefab(prefab);
	//auto prefab2 = SerializeToPrefab(*copy);
	//auto result = SerializePrefab(prefab);

	//std::cout << result << "\n" << SerializePrefab(prefab2);

	//auto prefab3 = DeserializePrefab(result);
	////auto prefab3 = DeserializePrefab(result2);
	return 0;
}