module;

#include <cstdlib>
#include <cstdint>
#include <new>
#include <concepts>
#include <bit>
#include <cassert>

export module InsanityFramework.Memory;

namespace InsanityFramework
{
	void* AlignedAlloc(size_t alignment, size_t size)
	{
		return _aligned_malloc(size, alignment);
	}

	void AlignedFree(void* ptr)
	{
		_aligned_free(ptr);
	}

	void* OffsetPointer(void* ptr, std::ptrdiff_t offset)
	{
		return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) + offset);
	}

	template<class Ty>
	void* OffsetPointerAs(void* ptr, std::ptrdiff_t offset)
	{
		return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) + sizeof(Ty) * offset);
	}

	//Aligns value to the next power of 2
	template<std::unsigned_integral Ty>
	constexpr Ty CielingPow2(Ty value)
	{
		value--;
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;

		if constexpr(sizeof(Ty) >= 2)
			value |= value >> 8;

		if constexpr(sizeof(Ty) >= 4)
			value |= value >> 16;

		if constexpr(sizeof(Ty) >= 8)
			value |= value >> 32;
		value++;

		return value;
	}

	//Aligns address to the next power of 2
	constexpr void* CielingPow2(void* value)
	{
		return reinterpret_cast<void*>(CielingPow2(std::bit_cast<uintptr_t>(value)));
	}

	//Aligns value to the previous power of 2
	template<std::unsigned_integral Ty>
	constexpr Ty FloorPow2(Ty value)
	{
		return CielingPow2(value) >> 1;
	}	

	//Aligns address to the previous power of 2
	constexpr void* FloorPow2(void* value)
	{
		return reinterpret_cast<void*>(FloorPow2(std::bit_cast<uintptr_t>(value)));
	}

	//Aligns value to the next custom alignment which itself is a power of 2
	//Ex: value = 17, alignment = 16, return = 32
	//Ex: value = 17, alignment = 8, return = 24
	template<std::unsigned_integral Ty>
	constexpr Ty AlignCeilPow2(Ty value, Ty alignment)
	{
		assert(alignment % 2 == 0);
		return value + (~value & alignment);
	}

	//Aligns value to the next custom alignment
	//Ex: value = 17, alignment = 25, return = 25
	//Ex: value = 17, alignment = 5, return = 20
	template<std::unsigned_integral Ty>
	constexpr Ty AlignCeil(Ty value, Ty alignment)
	{
		if(alignment % 2 == 0)
			return AlignCeilPow2(value, alignment);
		else
			return value + (value % alignment);
	}
		
	//Aligns value to the previous custom alignment which itself is a power of 2
	//Ex: value = 28, alignment = 16, return = 16
	//Ex: value = 28, alignment = 8, return = 24
	template<std::unsigned_integral Ty>
	constexpr Ty AlignFloorPow2(Ty value, Ty alignment)
	{
		return AlignCeilPow2(value, alignment) - alignment;
	}

	//Aligns value to the previous custom alignment
	//Ex: value = 34, alignment = 7, return = 28
	//Ex: value = 34, alignment = 10, return = 30
	template<std::unsigned_integral Ty>
	constexpr Ty AlignFloor(Ty value, Ty alignment)
	{
		return AlignCeil(value, alignment) - alignment;
	}

	constexpr void* AlignCeilPow2(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignCeilPow2(std::bit_cast<std::uintptr_t>(value), alignment));
	}

	constexpr void* AlignCeil(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignCeil(std::bit_cast<std::uintptr_t>(value), alignment));
	}
		
	constexpr void* AlignFloorPow2(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignFloorPow2(std::bit_cast<std::uintptr_t>(value), alignment));
	}

	constexpr void* AlignFloor(void* value, std::size_t alignment)
	{
		return reinterpret_cast<void*>(AlignFloor(std::bit_cast<std::uintptr_t>(value), alignment));
	}
}