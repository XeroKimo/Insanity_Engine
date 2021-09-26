#include "pch.h"
#include "CppUnitTest.h"
#include "glm-master/glm/gtc/matrix_transform.hpp"
#include <sstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace InsanityEngine::Math::Types;
using namespace InsanityEngine;

void PrintMatrix(const glm::mat4x4& mat)
{
    std::stringstream stream;

    for(size_t y = 0; y < 4; y++)
    {
        for(size_t x = 0; x < 4; x++)
            stream << mat[y][x] << " ";

        stream << "\n";
    }
    stream << "\n";

    Logger::WriteMessage(stream.str().c_str());

}

void PrintMatrix(const Matrix4x4f& mat)
{
    std::stringstream stream;

    for(size_t y = 0; y < 4; y++)
    {
        for(size_t x = 0; x < 4; x++)
            stream << mat(y, x) << " ";

        stream << "\n";
    }
    stream << "\n";

    Logger::WriteMessage(stream.str().c_str());

}

namespace INSANITYMATHTEST
{
    TEST_CLASS(INSANITYMATHTEST)
    {
    public:

        TEST_METHOD(VectorAddTest)
        {
            Vector3f one{ 1, 2, 3 };
            Vector3f two{ 4, 5, 6 };
            Vector3f three = one + two;

            glm::vec3 gOne{ 1, 2, 3 };
            glm::vec3 gTwo{ 4, 5, 6 };
            glm::vec3 gThree = gOne + gTwo;

            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }


        }



        TEST_METHOD(VectorSubtractTest)
        {
            Vector3f one{ 1, 2, 3 };
            Vector3f two{ 4, 5, 6 };
            Vector3f three = one - two;

            glm::vec3 gOne{ 1, 2, 3 };
            glm::vec3 gTwo{ 4, 5, 6 };
            glm::vec3 gThree = gOne - gTwo;

            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }
        }



        TEST_METHOD(VectorMulTest)
        {
            Vector3f one{ 1, 2, 3 };
            Vector3f two{ 4, 5, 6 };
            Vector3f three = one * two;

            glm::vec3 gOne{ 1, 2, 3 };
            glm::vec3 gTwo{ 4, 5, 6 };
            glm::vec3 gThree = gOne * gTwo;

            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }
        }

        TEST_METHOD(VectorDivTest)
        {
            Vector3f one{ 1, 2, 3 };
            Vector3f two{ 4, 5, 6 };
            Vector3f three = one / two;

            glm::vec3 gOne{ 1, 2, 3 };
            glm::vec3 gTwo{ 4, 5, 6 };
            glm::vec3 gThree = gOne / gTwo;

            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }
        }


        TEST_METHOD(VectorFunctionsTest)
        {
            Vector3f one{ 1, 2, 3 };
            Vector3f two{ 4, 5, 6 };
            glm::vec3 gOne{ 1, 2, 3 };
            glm::vec3 gTwo{ 4, 5, 6 };


            Assert::AreEqual(Math::Functions::Vector::Magnitude(one), glm::length(gOne));

            Assert::AreEqual(Math::Functions::Vector::Dot(one, two), glm::dot(gOne, gTwo));

            Vector3f three = Math::Functions::Vector::Cross(one, two);
            glm::vec3 gThree = glm::cross(gOne, gTwo);

            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }

        }


        TEST_METHOD(MatrixEqualityTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            Matrix4x4f two = one;


            Assert::AreEqual(one == two, true);
        }


        TEST_METHOD(MatrixAddTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            Matrix4x4f two
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            Matrix4x4f three = one + two;

            glm::mat4x4 gOne
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            glm::mat4x4 gTwo
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            glm::mat4x4 gThree = gOne + gTwo;

            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(gThree[y][x], three(y, x));
            }
        }



        TEST_METHOD(MatrixSubtractTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            Matrix4x4f two
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            Matrix4x4f three = one - two;

            glm::mat4x4 gOne
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            glm::mat4x4 gTwo
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            glm::mat4x4 gThree = gOne - gTwo;

            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(gThree[y][x], three(y, x));
            }
        }



        TEST_METHOD(MatrixMulTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            Matrix4x4f two
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            Matrix4x4f three = one * two;

            glm::mat4x4 gOne
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };
            glm::mat4x4 gTwo
            {
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            };
            glm::mat4x4 gThree = gTwo * gOne;

            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(gThree[y][x], three(y, x));
            }

            Matrix<float, 2, 3> f1
            {
                1, 2, 3,
                4, 5, 6
            };
            Matrix<float, 3, 2> f2
            {
                1, 2, 
                3, 4,
                5, 6
            };

            Matrix<float, 3, 3> f3 = f2 * f1;

            glm::mat2x3 gf1
            {
                1, 2, 3,
                4, 5, 6
            };

            glm::mat3x2 gf2
            {
                1, 2,
                3, 4,
                5, 6
            };

            glm::mat3x3 gf3 = gf1 * gf2;

            for(size_t y = 0; y < Matrix<float, 3, 3> ::row_count; y++)
            {
                for(size_t x = 0; x < Matrix<float, 3, 3> ::column_count; x++)
                    Assert::AreEqual(gf3[y][x], f3(y, x));
            }

        }



        TEST_METHOD(MatrixVectorTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };

            Vector4f two{ 5, 6, 7, 8 };
            Vector4f three = one * two;

            glm::mat4x4 gOne
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };

            glm::vec4 gTwo{ 5, 6, 7, 8 };
            glm::vec4 gThree = gOne * gTwo;


            for(size_t i = 0; i < three.size; i++)
            {
                Assert::AreEqual(gThree[i], three[i]);
            }

        }

        TEST_METHOD(MatrixTranslationTest)
        {

            Matrix4x4f one
            {
                1, 0, 0, 1,
                0, 1, 0, 2,
                0, 0, 1, 3,
                0, 0, 0, 1
            };

            Matrix4x4f two
            {
                1, 0, 0, 9,
                0, 1, 0, 8,
                0, 0, 1, 7,
                0, 0, 0, 1
            };

            Matrix4x4f three = two * one;

            glm::mat4x4 gOne
            {
                1, 0, 0, 1,
                0, 1, 0, 2,
                0, 0, 1, 3,
                0, 0, 0, 1
            };

            glm::mat4x4 gTwo
            {
                1, 0, 0, 9,
                0, 1, 0, 8,
                0, 0, 1, 7,
                0, 0, 0, 1
            };


            glm::mat4x4 gThree = gTwo * gOne;



            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(gThree[y][x], three(y, x));
            }
        }


        TEST_METHOD(MatrixFunctionTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };

            Matrix4x4f two = Math::Functions::Matrix::TransposeCopy(one);

            glm::mat4x4 gOne =
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };

            glm::mat4x4 gTwo = glm::transpose(gOne);

            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(gTwo[y][x], two(y, x));
            }
        }


        TEST_METHOD(MatrixIdentityTest)
        {
            Matrix4x4f one
            {
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            };

            Matrix4x4f two = Matrix4x4f::Identity();
            Matrix4x4f three = one * Matrix4x4f::Identity();


            for(size_t y = 0; y < Matrix4x4f::row_count; y++)
            {
                for(size_t x = 0; x < Matrix4x4f::column_count; x++)
                    Assert::AreEqual(one(y, x), three(y, x));
            }
        }

        TEST_METHOD(TrigTest)
        {
            Degrees<float> d = Degrees(180.f);
            Radians<float> r = d.ToRadians();

            Assert::AreEqual(r.Data(), d.Data() / 180.f * Math::Constants::pi<float>);
        }

    };
}
