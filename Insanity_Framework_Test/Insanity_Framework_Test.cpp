#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
import InsanityFramework.ECS.Scene;
import xk.Math;

using namespace InsanityFramework;
using namespace xk::Math;

namespace InsanityFrameworkTest
{
	TEST_CLASS(ParentingTests)
	{
	public:
		
		TEST_METHOD(AddingParent)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			Assert::IsTrue(t2.GetParent() == &t1);
			Assert::IsTrue(t1.GetChildren()[0] == &t2);
		}
		
		TEST_METHOD(RemovingParent)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);
			t2.SetParent(nullptr);

			Assert::IsTrue(t2.GetParent() == nullptr);
			Assert::IsTrue(t1.GetChildren().size() == 0);
		}
		
		TEST_METHOD(RemovingParentViaDestructor)
		{
			TransformNode* t1 = new TransformNode();
			TransformNode t2;

			t2.SetParent(t1);
			delete t1;

			Assert::IsTrue(t2.GetParent() == nullptr);
		}
		
		TEST_METHOD(RemovingChildViaDestructor)
		{
			TransformNode t1;
			{
				TransformNode t2;
				t2.SetParent(&t1);
			}

			Assert::IsTrue(t1.GetChildren().size() == 0);
		}
		
		TEST_METHOD(TwoStacksDeep)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);
			t3.SetParent(&t2);

			Assert::IsTrue(t2.GetParent() == &t1);
			Assert::IsTrue(t1.GetChildren()[0] == &t2);

			Assert::IsTrue(t3.GetParent() == &t2);
			Assert::IsTrue(t2.GetChildren()[0] == &t3);
		}
		
		TEST_METHOD(ReparentingTest)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);
			t3.SetParent(&t2);

			Assert::IsTrue(t2.GetParent() == &t1);
			Assert::IsTrue(t1.GetChildren()[0] == &t2);

			Assert::IsTrue(t3.GetParent() == &t2);
			Assert::IsTrue(t2.GetChildren()[0] == &t3);

			t3.SetParent(&t1);

			Assert::IsTrue(t3.GetParent() == &t1);
			Assert::IsTrue(t1.GetChildren()[1] == &t3);
		}
		
		TEST_METHOD(ReparentingViaDestructor)
		{
			TransformNode t1;
			TransformNode* t2 = new TransformNode();
			TransformNode t3;

			t2->SetParent(&t1);
			t3.SetParent(t2);

			delete t2;

			Assert::IsTrue(t3.GetParent() == &t1);
			Assert::IsTrue(t1.GetChildren()[0] == &t3);
		}
		
		TEST_METHOD(CyclicDetectionTest)
		{
			Assert::ExpectException<std::exception>([t1 = TransformNode{}, t2 = TransformNode{}]() mutable
				{
					t1.SetParent(&t2);
					t2.SetParent(&t1);
				});
		}
		
		TEST_METHOD(ComplexCyclicDetectionTest)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);
			t3.SetParent(&t2);

			Assert::ExpectException<std::exception>([&]() mutable
				{
					t2.SetParent(&t3);
				});

			Assert::ExpectException<std::exception>([&]() mutable
				{
					t1.SetParent(&t3);
				});
		}
		
		TEST_METHOD(SelfParentingTest)
		{
			Assert::ExpectException<std::exception>([t1 = TransformNode{}]() mutable
				{
					t1.SetParent(&t1);
				});
		}
	};

	TEST_CLASS(WorldTransformationPositionTests)
	{
	public:
		
		TEST_METHOD(ParentPositionChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);
			t1.WorldTransform().Position() = { 10.f, 10.f };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Position().Get() != t2.LocalTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.LocalTransform().Position().Get() == Vector<float, 2>{});
		}
		
		TEST_METHOD(ChildPositionChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t2.WorldTransform().Position() = { 10.f, 10.f };
			Assert::IsTrue(t1.WorldTransform().Position().Get() == xk::Math::Vector<float, 2>{0, 0});
			Assert::IsTrue(t2.WorldTransform().Position().Get() == xk::Math::Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t1.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{});
			Assert::IsTrue(t2.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 10.f, 10.f });
		}
		
		TEST_METHOD(ParentAndChildSamePositionChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };
			t2.WorldTransform().Position() = { 10.f, 10.f };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 0.f, 0.f });
		}
		
		TEST_METHOD(ParentAndChildDifferentPositionChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };
			t2.WorldTransform().Position() = { 0.f, 0.f };
			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get() - t2.LocalTransform().Position().Get());
			Assert::IsTrue(t2.WorldTransform().Position().Get() == Vector<float, 2>{ 0.f, 0.f });
			Assert::IsTrue(t1.LocalTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ -10.f, -10.f });
		}
		
		TEST_METHOD(UnparentChangePositionKeepWorldTransform)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };

			t2.SetParentKeepWorldTransform(nullptr);

			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 10.f, 10.f });
		}
		
		TEST_METHOD(UnparentChangePositionKeepLocalTransform)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };

			t2.SetParent(nullptr);

			Assert::IsTrue(t1.WorldTransform().Position().Get() != t2.WorldTransform().Position().Get());
			Assert::IsTrue(t2.WorldTransform().Position().Get() == t2.LocalTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.LocalTransform().Position().Get() == xk::Math::Vector<float, 2>{ 0.f, 0.f });
		}
		
		TEST_METHOD(ReparentChangePositionKeepWorldTransform)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };
			t3.WorldTransform().Position() =  { -40, 100 };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get());
			t2.SetParentKeepWorldTransform(&t3);


			Assert::IsTrue(t2.WorldTransform().Position().Get() != t3.WorldTransform().Position().Get());
			Assert::IsTrue(t1.WorldTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f});
			Assert::IsTrue(t3.WorldTransform().Position().Get() == Vector<float, 2>{ -40, 100 });
			Assert::IsTrue(t2.WorldTransform().Position().Get() == t1.WorldTransform().Position().Get());
			Assert::IsTrue(t2.LocalTransform().Position().Get() == Vector<float, 2>{ 50, -90 });
		}
		
		TEST_METHOD(ReparentChangePositionKeepLocalTransform)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = { 10.f, 10.f };
			t3.WorldTransform().Position() =  { -40, 100 };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == t2.WorldTransform().Position().Get());
			t2.SetParent(&t3);

			Assert::IsTrue(t2.WorldTransform().Position().Get() == t3.WorldTransform().Position().Get());

			Assert::IsTrue(t1.WorldTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f});
			Assert::IsTrue(t3.WorldTransform().Position().Get() == Vector<float, 2>{ -40, 100 });
		}

		TEST_METHOD(AccumulationTest)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Position() = Vector<float, 2>{ 10, 10 };
			t2.WorldTransform().Position() += Vector<float, 2>{ 20, 20 };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.WorldTransform().Position().Get() == Vector<float, 2>{ 30.f, 30.f });
		}

		TEST_METHOD(SubtractionTest)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);
			t1.WorldTransform().Position() = Vector<float, 2>{ 10, 10 };
			t2.WorldTransform().Position() -= Vector<float, 2>{ 20, 20 };

			Assert::IsTrue(t1.WorldTransform().Position().Get() == Vector<float, 2>{ 10.f, 10.f });
			Assert::IsTrue(t2.WorldTransform().Position().Get() == Vector<float, 2>{ -10.f, -10.f });
		}
	};

	TEST_CLASS(WorldTransformationRotationTests)
	{
	public:
		
		TEST_METHOD(ParentRotationChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Rotation() = { 10.f };

			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation());
			Assert::IsTrue(t1.LocalTransform().Rotation() != t2.LocalTransform().Rotation());
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{});
		}
		
		TEST_METHOD(ChildRotationChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t2.WorldTransform().Rotation() = { 10.f };
			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 0 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ 10.f });
		}
		
		TEST_METHOD(ParentAndChildSameRotationChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);


			t1.WorldTransform().Rotation() = { 10.f };
			t2.WorldTransform().Rotation() = { 10.f };

			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation());
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ 0.f });
		}
		
		TEST_METHOD(ParentAndChildDifferentRotationChange)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);


			t1.WorldTransform().Rotation() = { 10.f };
			t2.WorldTransform().Rotation() = { 0.f };
			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation().Get() - t2.LocalTransform().Rotation());
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ 0.f });
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ -10.f });
		}
		
		TEST_METHOD(UnparentChangeRotationKeepWorldTransform)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);


			t1.WorldTransform().Rotation() = { 10.f };

			t2.SetParentKeepWorldTransform(nullptr);

			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation());
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ 10.f });
		}
		
		TEST_METHOD(UnparentChangeRotationKeepLocalTransform)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Rotation() = { 10.f };

			t2.SetParent(nullptr);

			Assert::IsTrue(t1.WorldTransform().Rotation() != t2.WorldTransform().Rotation());
			Assert::IsTrue(t2.WorldTransform().Rotation().Get() == t2.LocalTransform().Rotation());
			Assert::IsTrue(t1.LocalTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ 0.f });
		}
		
		TEST_METHOD(ReparentChangeRotationKeepWorldTransform)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);

			t1.WorldTransform().Rotation() = { 10.f };
			t3.WorldTransform().Rotation() = { -40 };

			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation());
			t2.SetParentKeepWorldTransform(&t3);


			Assert::IsTrue(t2.WorldTransform().Rotation() != t3.WorldTransform().Rotation());
			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t3.WorldTransform().Rotation() == Degree<float>{ -40 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == t1.WorldTransform().Rotation());
			Assert::IsTrue(t2.LocalTransform().Rotation() == Degree<float>{ 50 });
		}
		
		TEST_METHOD(ReparentChangeRotationKeepLocalTransform)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);


			t1.WorldTransform().Rotation() = { 10.f };
			t3.WorldTransform().Rotation() = { -40 };

			Assert::IsTrue(t1.WorldTransform().Rotation() == t2.WorldTransform().Rotation());
			t2.SetParent(&t3);

			Assert::IsTrue(t2.WorldTransform().Rotation() == t3.WorldTransform().Rotation());

			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t3.WorldTransform().Rotation() == Degree<float>{ -40 });
		}

		TEST_METHOD(AccumulationTest)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.WorldTransform().Rotation() = Degree<float>{ 10 };
			t2.WorldTransform().Rotation() += Degree<float>{ 20};

			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ 30.f });
		}

		TEST_METHOD(SubtractionTest)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);


			t1.WorldTransform().Rotation() = Degree<float>{ 10 };
			t2.WorldTransform().Rotation() -= Degree<float>{ 20};

			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10.f });
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ -10.f });
		}
	};

	TEST_CLASS(LocalTransformationTests)
	{
		TEST_METHOD(ProxyEquivalence)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };
			t1.LocalTransform().Rotation() = Degree<float>{ 10 };

			t2.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };
			t2.LocalTransform().Rotation() = Degree<float>{ 10 };

			Assert::IsTrue(t1.LocalTransform().Position() == t1.LocalTransform().Position().Get());
			Assert::IsTrue(t1.LocalTransform().Rotation() == t1.LocalTransform().Rotation());

			Assert::IsTrue(t2.LocalTransform().Position() == t2.LocalTransform().Position().Get());
			Assert::IsTrue(t2.LocalTransform().Rotation() == t2.LocalTransform().Rotation());
		}

		TEST_METHOD(OneStackDeepLocation)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };
			t2.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };

			Assert::IsTrue(t1.WorldTransform().Position() == Vector<float, 2>{ 10, 10 });
			Assert::IsTrue(t2.WorldTransform().Position() == Vector<float, 2>{ 20, 20 });
			Assert::IsTrue(t2.WorldTransform().Position() == t1.LocalTransform().Position() + t2.LocalTransform().Position());
			Assert::IsTrue(t2.WorldTransform().Position() == t1.WorldTransform().Position().Get() + t2.LocalTransform().Position());
		}

		TEST_METHOD(OneStackDeepRotation)
		{
			TransformNode t1;
			TransformNode t2;

			t2.SetParent(&t1);

			t1.LocalTransform().Rotation() = Degree<float>{ 10 };
			t2.LocalTransform().Rotation() = Degree<float>{ 10 };

			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ 20 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == t1.LocalTransform().Rotation() + t2.LocalTransform().Rotation());
			Assert::IsTrue(t2.WorldTransform().Rotation() == t1.WorldTransform().Rotation().Get() + t2.LocalTransform().Rotation());
		}

		TEST_METHOD(TwoStackDeepLocation)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);
			t3.SetParent(&t2);

			t1.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };
			t2.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };
			t3.LocalTransform().Position() = Vector<float, 2>{ 10, 10 };

			Assert::IsTrue(t1.WorldTransform().Position() == Vector<float, 2>{ 10, 10 });
			Assert::IsTrue(t2.WorldTransform().Position() == Vector<float, 2>{ 20, 20 });
			Assert::IsTrue(t3.WorldTransform().Position() == Vector<float, 2>{ 30, 30 });
			Assert::IsTrue(t2.WorldTransform().Position() == t1.LocalTransform().Position() + t2.LocalTransform().Position());
			Assert::IsTrue(t2.WorldTransform().Position() == t1.WorldTransform().Position().Get() + t2.LocalTransform().Position());
			Assert::IsTrue(t3.WorldTransform().Position() == t1.LocalTransform().Position() + t2.LocalTransform().Position() + t3.LocalTransform().Position());
			Assert::IsTrue(t3.WorldTransform().Position() == t2.WorldTransform().Position().Get() + t3.LocalTransform().Position());
		}

		TEST_METHOD(TwoStackDeepRotation)
		{
			TransformNode t1;
			TransformNode t2;
			TransformNode t3;

			t2.SetParent(&t1);
			t3.SetParent(&t2);

			t1.LocalTransform().Rotation() = Degree<float>{ 10 };
			t2.LocalTransform().Rotation() = Degree<float>{ 10 };
			t3.LocalTransform().Rotation() = Degree<float>{ 10 };

			Assert::IsTrue(t1.WorldTransform().Rotation() == Degree<float>{ 10 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == Degree<float>{ 20 });
			Assert::IsTrue(t3.WorldTransform().Rotation() == Degree<float>{ 30 });
			Assert::IsTrue(t2.WorldTransform().Rotation() == t1.LocalTransform().Rotation() + t2.LocalTransform().Rotation());
			Assert::IsTrue(t2.WorldTransform().Rotation() == t1.WorldTransform().Rotation().Get() + t2.LocalTransform().Rotation());
			Assert::IsTrue(t3.WorldTransform().Rotation() == t1.LocalTransform().Rotation() + t2.LocalTransform().Rotation() + t3.LocalTransform().Rotation());
			Assert::IsTrue(t3.WorldTransform().Rotation() == t2.WorldTransform().Rotation().Get() + t3.LocalTransform().Rotation());
		}
	};
}
