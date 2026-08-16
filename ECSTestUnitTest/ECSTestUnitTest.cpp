#include "pch.h"
#include "CppUnitTest.h"
#include <Windows.h>
#include <nlohmann/json.hpp>

import ECSTest;
import std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static std::wstring ToString(const std::type_info& q) 
{
	std::string_view typeName = q.name();
	std::wstring wTypeName;
	wTypeName.resize(typeName.size());

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, typeName.data(), typeName.size(), wTypeName.data(), wTypeName.size());
	return wTypeName;
}

static std::wstring ToString(const GameObject& q) 
{
	std::string_view typeName = q.name;
	std::wstring wTypeName;
	wTypeName.resize(typeName.size());

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, typeName.data(), typeName.size(), wTypeName.data(), wTypeName.size());
	return wTypeName;
}

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


namespace ECSTestUnitTest
{
	TEST_CLASS(ECSBasicGameObjectTests)
	{
	private:
		GameObject a;
		GameObject b;
		GameObject c;
		GameObject d;

		Prefab<GameObject> prefab;

	public:
		ECSBasicGameObjectTests()
		{
			RegisterGameObjectType<GameObject>();

			a.name = "Fub";
			b.name = "Dub";
			c.name = "Cub";
			d.name = "Rub";
			b.SetParent(&a);
			c.SetParent(&b);
			d.SetParent(&a);

			prefab = SerializeToPrefab(a);
		}

		//Checks to see if the prefab created by SerializeToPrefab() contains the same data as the GameObjects used to create it
		TEST_METHOD(PrefabEquivalenceTest)
		{
			Assert::IsTrue(a.name == prefab.data->name, L"Root name mismatched");
			Assert::AreSame(typeid(a), *prefab.data->type, L"Root type mismatched");

			struct FlattenStructure
			{
				PrefabData<GameObject>* parentData;
				PrefabData<GameObject>* childData;
				GameObject* child;
			};

			std::queue<FlattenStructure> queue;

			for (auto [child, childData] = std::pair{ a.firstChild, prefab.data->children.begin() }; child && childData != prefab.data->children.end(); child = child->next, ++childData)
			{
				queue.push({ prefab.data.get(), childData->get(), child.get() });
			}

			Assert::IsTrue(prefab.data->children.size() == queue.size(), L"Queue size mismatched");

			while (!queue.empty())
			{
				auto [parentData, childData, child] = queue.front();
				queue.pop();

				Assert::IsTrue(child->parent->name == parentData->name ,  L"Parent name mismatched");
				Assert::IsTrue(child->name == childData->name  , L"Child name mismatched");
				Assert::AreSame(typeid(*child), *childData->type  , L"Child type mismatched");

				int oldSize = queue.size();
				for (auto [grandChild, grandChildData] = std::pair{ child->firstChild, childData->children.begin() }; grandChild && grandChildData != childData->children.end(); grandChild = grandChild->next, ++grandChildData)
				{
					queue.push({ childData, grandChildData->get(), grandChild.get() });
				}
				Assert::IsTrue(childData->children.size() == queue.size() - oldSize, L"Queue size mismatched");
			}
		}

		//Checks to see if spawning the prefab is equivalent to the original game object tree
		TEST_METHOD(PrefabSpawnEquivalenceTest)
		{
			std::unique_ptr<GameObject> go = SpawnPrefab(prefab);

			Assert::IsTrue(a.name == go->name, L"Root name mismatched");
			Assert::AreSame(typeid(a), typeid(*go), L"Root type mismatched");

			struct FlattenStructure
			{
				Handle<GameObject> child;
				Handle<GameObject> child2;
			};

			std::queue<FlattenStructure> queue;

			for (auto [child, child2] = std::pair{ a.firstChild, go->firstChild }; child && child2; child = child->next, child2 = child2->next)
			{
				queue.push({ child.get(), child2.get() });
			}

			Assert::IsTrue([&] 
			{
				int i = 0;
				auto child = a.firstChild;
				while (child)
				{
					i++;
					child = child->next;
				}

				return i;
			}() == queue.size(), L"Queue size mismatched");

			while (!queue.empty())
			{
				auto [child, child2] = queue.front();
				queue.pop();

				Assert::IsTrue(child->parent->name == child2->parent->name, L"Parent name mismatched");
				Assert::IsTrue(child->name == child2->name, L"Child name mismatched");
				Assert::AreSame(typeid(*child), typeid(*child2), L"Child type mismatched");

				int oldSize = queue.size();

				for (auto [grandChild, grandChild2] = std::pair{ child->firstChild, child2->firstChild }; grandChild && grandChild2; grandChild = grandChild->next, grandChild2 = grandChild2->next)
				{
					queue.push({ grandChild.get(), grandChild2.get() });
				}
				Assert::IsTrue([&c = child]
				{
					int i = 0;
					auto child = c->firstChild;
					while (child)
					{
						i++;
						child = child->next;
					}

					return i;
				}() == queue.size() - oldSize, L"Queue size mismatched");
			}
		}

		TEST_METHOD(PrefabDeserializationTest)
		{
			Prefab<GameObject> copy = DeserializePrefab(SerializePrefab(prefab));

			struct FlattenStructure
			{
				PrefabData<GameObject>* parent;
				PrefabData<GameObject>* child;
				PrefabData<GameObject>* parent2;
				PrefabData<GameObject>* child2;
			};
			std::queue<FlattenStructure> queue;

			Assert::AreSame(*prefab.data->type, *copy.data->type, L"Root type mismatched");
			Assert::IsTrue(prefab.data->id == copy.data->id, L"Root ID mismatched");
			Assert::IsTrue(prefab.data->name == copy.data->name, L"Root name mismatched");
			Assert::IsTrue(prefab.data->children.size() == copy.data->children.size(), L"Children count mismatched");
			for (auto [child, child2] = std::pair{ prefab.data->children.begin(), copy.data->children.begin() }; child != prefab.data->children.end() && child2 != copy.data->children.end(); ++child, ++child2)
			{
				queue.push({ prefab.data.get(), child->get(), copy.data.get(), child2->get() });
			}

			while (!queue.empty())
			{
				auto [parent, child, parent2, child2] = queue.front();
				queue.pop();

				Assert::AreSame(*child->type, *child2->type, L"Child type mismatched");
				Assert::IsTrue(child->id == child2->id, L"Child ID mismatched");
				Assert::IsTrue(child->name == child2->name, L"Child name mismatched");
				Assert::IsTrue(child->children.size() == child2->children.size(), L"Children count mismatched");
				for (auto [grandchild, grandchild2] = std::pair{ child->children.begin(), child2->children.begin() }; grandchild != child->children.end() && grandchild2 != child2->children.end(); ++grandchild, ++grandchild2)
				{
					queue.push({ child, grandchild->get(), child2, grandchild2->get() });
				}
			}
		}
	};

	TEST_CLASS(ECSDerivedGameObjectTests)
	{
	private:
		GameObject a;
		B b;
		C c;
		D d;

		Prefab<GameObject> prefab;

	public:
		ECSDerivedGameObjectTests()
		{
			RegisterGameObjectType<GameObject>();
			RegisterGameObjectType<B>();
			RegisterGameObjectType<C>();
			RegisterGameObjectType<D>();

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

			prefab = SerializeToPrefab(a);
		}
		
		//Checks to see if the prefab created by SerializeToPrefab() contains the same data as the GameObjects used to create it
		TEST_METHOD(PrefabEquivalenceTest)
		{
			Assert::IsTrue(a.name == prefab.data->name, L"Root name mismatched");
			Assert::AreSame(typeid(a), *prefab.data->type, L"Root type mismatched");

			struct FlattenStructure
			{
				PrefabData<GameObject>* parentData;
				PrefabData<GameObject>* childData;
				GameObject* child;
			};

			std::queue<FlattenStructure> queue;

			for (auto [child, childData] = std::pair{ a.firstChild, prefab.data->children.begin() }; child && childData != prefab.data->children.end(); child = child->next, ++childData)
			{
				queue.push({ prefab.data.get(), childData->get(), child.get() });
			}

			Assert::IsTrue(prefab.data->children.size() == queue.size(), L"Queue size mismatched");

			while (!queue.empty())
			{
				auto [parentData, childData, child] = queue.front();
				queue.pop();

				Assert::IsTrue(child->parent->name == parentData->name ,  L"Parent name mismatched");
				Assert::IsTrue(child->name == childData->name  , L"Child name mismatched");
				Assert::AreSame(typeid(*child), *childData->type  , L"Child type mismatched");
				if (dynamic_cast<PrefabData<C>*>(childData))
				{
					auto derived = static_cast<PrefabData<C>*>(childData);
					Assert::IsTrue(derived->something == c.something);
					Assert::IsTrue(derived->color == c.color);
				}
				else if (dynamic_cast<PrefabData<B>*>(childData))
				{
					auto derived = static_cast<PrefabData<B>*>(childData);
					Assert::IsTrue(derived->color == b.color);
				}
				else if(dynamic_cast<PrefabData<D>*>(childData))
				{
					auto derived = static_cast<PrefabData<D>*>(childData);
					Assert::IsTrue(derived->someB.id == parentData->children[0]->id);
				}

				int oldSize = queue.size();
				for (auto [grandChild, grandChildData] = std::pair{ child->firstChild, childData->children.begin() }; grandChild && grandChildData != childData->children.end(); grandChild = grandChild->next, ++grandChildData)
				{
					queue.push({ childData, grandChildData->get(), grandChild.get() });
				}
				Assert::IsTrue(childData->children.size() == queue.size() - oldSize, L"Queue size mismatched");
			}
		}

		//Checks to see if spawning the prefab is equivalent to the original game object tree
		TEST_METHOD(PrefabSpawnEquivalenceTest)
		{
			std::unique_ptr<GameObject> go = SpawnPrefab(prefab);

			Assert::IsTrue(a.name == go->name, L"Root name mismatched");
			Assert::AreSame(typeid(a), typeid(*go), L"Root type mismatched");

			struct FlattenStructure
			{
				Handle<GameObject> child;
				Handle<GameObject> child2;
			};

			std::queue<FlattenStructure> queue;

			for (auto [child, child2] = std::pair{ a.firstChild, go->firstChild }; child && child2; child = child->next, child2 = child2->next)
			{
				queue.push({ child.get(), child2.get() });
			}

			Assert::IsTrue([&] 
			{
				int i = 0;
				auto child = a.firstChild;
				while (child)
				{
					i++;
					child = child->next;
				}

				return i;
			}() == queue.size(), L"Queue size mismatched");

			while (!queue.empty())
			{
				auto [child, child2] = queue.front();
				queue.pop();

				Assert::IsTrue(child->parent->name == child2->parent->name, L"Parent name mismatched");
				Assert::IsTrue(child->name == child2->name, L"Child name mismatched");
				Assert::AreSame(typeid(*child), typeid(*child2), L"Child type mismatched");

				if (auto [dc, dc2] = std::pair{ child.As<C>(), child2.As<C>() }; dc && dc2)
				{
					Assert::IsTrue(dc->something == dc2->something);
					Assert::IsTrue(dc->color == dc2->color);
				}
				if (auto [db, db2] = std::pair{ child.As<B>(), child2.As<B>() }; db && db2)
				{
					Assert::IsTrue(db->color == db2->color);
				}
				if (auto [dd, dd2] = std::pair{ child.As<D>(), child2.As<D>() }; dd && dd2)
				{
					Assert::IsFalse(dd->someB.IsID());
					Assert::IsFalse(dd2->someB.IsID());

					if (!dd->someB.IsID() && !dd2->someB.IsID())
					{
						Assert::AreSame<GameObject>(*dd->parent->firstChild, *dd->someB);
						Assert::AreSame<GameObject>(*dd2->parent->firstChild, *dd2->someB);
					}
				}

				int oldSize = queue.size();

				for (auto [grandChild, grandChild2] = std::pair{ child->firstChild, child2->firstChild }; grandChild && grandChild2; grandChild = grandChild->next, grandChild2 = grandChild2->next)
				{
					queue.push({ grandChild.get(), grandChild2.get() });
				}
				Assert::IsTrue([&c = child]
				{
					int i = 0;
					auto child = c->firstChild;
					while (child)
					{
						i++;
						child = child->next;
					}

					return i;
				}() == queue.size() - oldSize, L"Queue size mismatched");
			}
		}

		//Checks to see if prefab can be turned into something that can be written to a file and back and still be correct
		TEST_METHOD(PrefabDeserializationTest)
		{
			Prefab<GameObject> copy = DeserializePrefab(SerializePrefab(prefab));

			struct FlattenStructure
			{
				PrefabData<GameObject>* parent;
				PrefabData<GameObject>* child;
				PrefabData<GameObject>* parent2;
				PrefabData<GameObject>* child2;
			};
			std::queue<FlattenStructure> queue;

			Assert::AreSame(*prefab.data->type, *copy.data->type, L"Root type mismatched");
			Assert::IsTrue(prefab.data->id == copy.data->id, L"Root ID mismatched");
			Assert::IsTrue(prefab.data->name == copy.data->name, L"Root name mismatched");
			Assert::IsTrue(prefab.data->children.size() == copy.data->children.size(), L"Children count mismatched");
			for (auto [child, child2] = std::pair{ prefab.data->children.begin(), copy.data->children.begin() }; child != prefab.data->children.end() && child2 != copy.data->children.end(); ++child, ++child2)
			{
				queue.push({ prefab.data.get(), child->get(), copy.data.get(), child2->get() });
			}

			while (!queue.empty())
			{
				auto [parent, child, parent2, child2] = queue.front();
				queue.pop();

				Assert::AreSame(*child->type, *child2->type, L"Child type mismatched");
				Assert::IsTrue(child->id == child2->id, L"Child ID mismatched");
				Assert::IsTrue(child->name == child2->name, L"Child name mismatched");
				Assert::IsTrue(child->children.size() == child2->children.size(), L"Children count mismatched");
				for (auto [grandchild, grandchild2] = std::pair{ child->children.begin(), child2->children.begin() }; grandchild != child->children.end() && grandchild2 != child2->children.end(); ++grandchild, ++grandchild2)
				{
					queue.push({ child, grandchild->get(), child2, grandchild2->get() });
				}
			}
		}
	};
}
