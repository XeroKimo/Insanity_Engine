#pragma once

#include "MathConcepts.h"

namespace InsanityEngine::Math::Types
{
    template<Concepts::Arithmetic T>
    struct Scalar
    {
    public:
        T value = 0;

    public:
        constexpr Scalar() = default;
        constexpr Scalar(T value) : value(value) {}


    };
}