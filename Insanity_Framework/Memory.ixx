module;

#include <cstdlib>
#include <cstdint>
#include <new>
#include <concepts>
#include <bit>
#include <cassert>
#include <limits>

export module InsanityFramework.Memory;

namespace InsanityFramework
{
	export void* IncrementPointer(void* ptr, std::size_t offset)
	{
		return static_cast<std::byte*>(ptr) + offset;
	}

	export void* DecrementPointer(void* ptr, std::size_t offset)
	{
		return static_cast<std::byte*>(ptr) - offset;
	}

	export template<class Ty>
	void* IncrementPointerAs(void* ptr, std::size_t offset)
	{
		return static_cast<std::byte*>(ptr) + offset * sizeof(Ty);
	}

	export template<class Ty>
	void* DecrementPointerAs(void* ptr, std::size_t offset)
	{
		return static_cast<std::byte*>(ptr) - offset * sizeof(Ty);
	}

	export std::ptrdiff_t OffsetOf(void* a, void* b)
	{
		return static_cast<std::byte*>(b) - static_cast<std::byte*>(a);
	}

	//Aligns value to the next power of 2
	//Ex: value = 17, return = 32
	//Ex: value = 19, return = 32
	//Ex: value = 316, return = 512
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignNextPow2(Ty value)
	{
		return std::bit_ceil(value);
	}

	//Aligns address to the next power of 2
	//Ex: value = 17, return = 32
	//Ex: value = 19, return = 32
	//Ex: value = 316, return = 512
	export void* AlignNextPow2(void* value)
	{
		return std::bit_cast<void*>(AlignNextPow2(std::bit_cast<std::uintptr_t>(value)));
	}

	//Aligns value to the previous power of 2
	//Ex: value = 17, return = 16
	//Ex: value = 19, return = 16
	//Ex: value = 316, return = 256
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignPreviousPow2(Ty value)
	{
		return std::bit_floor(value);
	}	

	//Aligns address to the previous power of 2
	//Ex: value = 17, return = 16
	//Ex: value = 19, return = 16
	//Ex: value = 316, return = 256
	export void* AlignPreviousPow2(void* value)
	{
		return std::bit_cast<void*>(AlignPreviousPow2(std::bit_cast<uintptr_t>(value)));
	}

	//Aligns value to the next custom alignment which itself is a power of 2
	//Ex: value = 17, alignment = 16, return = 32
	//Ex: value = 17, alignment = 8, return = 24
	//Ex: value = 16, alignment = 16, return 16
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignCeilPow2(Ty value, Ty alignment)
	{
		assert(std::has_single_bit(alignment));
		return value + (std::numeric_limits<Ty>::max() - (value - 1) & (alignment - 1));
	}

	static_assert(AlignCeilPow2(17u, 16u) == 32);
	static_assert(AlignCeilPow2(17u, 8u) == 24);
	static_assert(AlignCeilPow2(16u, 16u) == 16);

	//Aligns value to the next custom alignment
	//Ex: value = 17, alignment = 25, return = 25
	//Ex: value = 17, alignment = 5, return = 20
	//Ex: value = 17, alignment = 17, return = 17
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignCeil(Ty value, Ty alignment)
	{
		if(alignment % 2 == 0)
			return AlignCeilPow2(value, alignment);
		else
			return (value != alignment) ? 
				value + alignment - (value % alignment) :
				value + (value % alignment);
	}

	static_assert(AlignCeil(17u, 25u) == 25);
	static_assert(AlignCeil(17u, 5u) == 20);
	static_assert(AlignCeil(17u, 17u) == 17);
		
	//Aligns value to the previous custom alignment which itself is a power of 2
	//Ex: value = 28, alignment = 16, return = 16
	//Ex: value = 28, alignment = 8, return = 24
	//Ex: value = 16, alignment = 16, return 16
	//Ex: value = 14, alignment = 16, return 0
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignFloorPow2(Ty value, Ty alignment)
	{
		return AlignCeilPow2(value + 1, alignment) - alignment;
	}

	static_assert(AlignFloorPow2(28u, 16u) == 16);
	static_assert(AlignFloorPow2(28u, 8u) == 24);
	static_assert(AlignFloorPow2(16u, 16u) == 16);
	static_assert(AlignFloorPow2(14u, 16u) == 0);

	//Aligns value to the previous custom alignment
	//Ex: value = 34, alignment = 7, return = 28
	//Ex: value = 34, alignment = 10, return = 30
	//Ex: value = 27, alignment = 27, return = 27
	//Ex: value = 26, alignment = 27, return = 0
	export template<std::unsigned_integral Ty>
	constexpr Ty AlignFloor(Ty value, Ty alignment)
	{
		return AlignCeil(value + (value == alignment), alignment) - alignment;
	}

	static_assert(AlignFloor(34u, 25u) == 25);
	static_assert(AlignFloor(34u, 5u) == 30);
	static_assert(AlignFloor(27u, 27u) == 27);
	static_assert(AlignFloor(26u, 27u) == 0);

	export void* AlignCeilPow2(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignCeilPow2(std::bit_cast<std::uintptr_t>(value), alignment));
	}

	export void* AlignCeil(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignCeil(std::bit_cast<std::uintptr_t>(value), alignment));
	}
		
	export void* AlignFloorPow2(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignFloorPow2(std::bit_cast<std::uintptr_t>(value), alignment));
	}

	export void* AlignFloor(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignFloor(std::bit_cast<std::uintptr_t>(value), alignment));
	}
}