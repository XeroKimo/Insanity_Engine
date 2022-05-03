#pragma once
#include <concepts>

namespace InsanityEngine::Utility
{
    template<std::integral Ty>
    constexpr Ty AlignCeiling(const Ty value, const Ty alignment)
    {
        const Ty alignCheck = value % alignment;
        if(alignCheck == 0)
            return value;

        const Ty powerOfTwoCheck = alignment % 2;

        if(powerOfTwoCheck == 0)
        {
            return (value + alignment) & ~(alignment - 1);
        }
        else
        {
            return value + alignment - powerOfTwoCheck;
        }
    }

    template<class Ty>
        requires (std::is_pointer_v<Ty> && sizeof(std::remove_pointer_t<Ty>) == 1)
    Ty AlignCeiling(Ty pointer, size_t alignment)
    {
        std::uintptr_t pointerOffset = reinterpret_cast<std::uintptr_t>(pointer);
        
        return reinterpret_cast<Ty>(AlignCeiling(pointerOffset, alignment));
    }

    template<std::integral Ty>
    constexpr Ty AlignFloor(const Ty value, const Ty alignment)
    {
        const Ty alignCheck = value % alignment;
        if(alignCheck == 0)
            return value;

        const Ty powerOfTwoCheck = alignment % 2;

        if(powerOfTwoCheck == 0)
        {
            return (value - alignCheck);
        }
        else
        {
            return value - alignCheck;
        }
    }

    template<class Ty>
        requires (std::is_pointer_v<Ty> && sizeof(std::remove_pointer_t<Ty>) == 1)
    Ty AlignFloor(Ty pointer, size_t alignment)
    {
        std::uintptr_t pointerOffset = reinterpret_cast<std::uintptr_t>(pointer);
        
        return reinterpret_cast<Ty>(AlignFloor(pointerOffset, alignment));
    }
}