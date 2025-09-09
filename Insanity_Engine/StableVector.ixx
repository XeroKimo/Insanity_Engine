module;

#include <cstdint>
#include <vector>
#include <bit>
#include <utility>

export module InsanityEngine.Container.StableVector;

namespace InsanityEngine
{

	export struct GenerationIndex
	{
		std::uint32_t generation : 8 = 0;
		std::uint32_t index : 24 = 0;

		bool operator==(const GenerationIndex&) const noexcept = default;
	};

	export struct GenerationHandle
	{
		std::uint32_t generation : 8 = 0;
		std::uint32_t index : 24 = 0;

		bool operator==(const GenerationHandle&) const noexcept = default;
	};

	export constexpr GenerationIndex null_index = { .generation = 0 };
	export constexpr GenerationHandle null_handle = { .generation = 0 };

	export template<class Ty>
		class StableVector
	{
		std::vector<Ty> values;
		std::vector<GenerationIndex> indexMapper;
		std::vector<std::uint32_t> freeIndices;

	public:
		GenerationHandle PushBack(Ty value)
		{
			values.push_back(std::move(value));
			GenerationIndex& index = [&]() -> GenerationIndex&
			{
				if (freeIndices.empty())
				{
					try
					{
						indexMapper.push_back({});
					}
					catch (...)
					{
						values.pop_back();
						throw;
					}

					indexMapper.back().generation = 1;
					return indexMapper.back();
				}
				else
				{
					auto& index = indexMapper[freeIndices.back()];
					freeIndices.pop_back();
					return index;
				}
			}();
			index.index = values.size() - 1;

			return { index.generation, static_cast<std::uint32_t>(std::distance(&*indexMapper.begin(), &index)) };
		}

		GenerationHandle MakeHandle(std::size_t i) const
		{
			const GenerationIndex& index = MapValueToIndex(values.at(i));
			return { index.generation, static_cast<std::uint32_t>(std::distance(&*indexMapper.begin(), &index)) };
		}

		bool Empty() const noexcept { return values.empty(); }
		std::size_t Size() const noexcept { return values.size(); }
		std::size_t Capacity() const noexcept { return values.capacity(); }

		void PopBack()
		{
			GenerationIndex& index = MapValueToIndex(values.back());
			values.pop_back();
			index.generation++;
			if (index.generation == 0)
				index.generation = 1;
			freeIndices.push_back(std::distance(&*indexMapper.begin(), &index));
		}

		Ty& Back() { return values.back(); }
		const Ty& Back() const { return values.back(); }

		void EraseHandle(GenerationHandle handle)
		{
			if (!IsValid(handle))
				return;

			EraseImpl(indexMapper[handle.index]);
		}

		void Erase(std::size_t index)
		{
			if (index >= values.size())
				return;

			EraseImpl(MapValueToIndex(values[index]));
		}

		bool IsValid(GenerationHandle handle)
		{
			return handle.generation != 0 && indexMapper[handle.index].generation == handle.generation;
		}

		Ty* HandleAt(GenerationHandle handle)
		{
			if (!IsValid(handle))
				return nullptr;

			return &values[indexMapper[handle.index].index];
		}

		const Ty* HandleAt(GenerationHandle handle) const
		{
			if (!IsValid(handle))
				return nullptr;

			return &values[indexMapper[handle.index].index];
		}

		Ty& At(std::size_t i)
		{
			return values.at(i);
		}

		const Ty& At(std::size_t i) const
		{
			return values.at(i);
		}

		auto begin()
		{
			return values.begin();
		}

		auto begin() const
		{
			return values.begin();
		}

		auto end()
		{
			return values.end();
		}

		auto end() const
		{
			return values.end();
		}

		void SwapElements(std::size_t l, std::size_t r)
		{
			auto& le = At(l);
			auto& re = At(r);
			auto& li = MapValueToIndex(le);
			auto& ri = MapValueToIndex(re);

			std::swap(le, re);
			std::uint32_t temp = li.index;
			li.index = ri.index;
			ri.index = temp;
		}
	private:
		void EraseImpl(GenerationIndex& currentIndex)
		{
			if (currentIndex.index < values.size() - 1)
			{
				GenerationIndex& backIndex = MapValueToIndex(values.back());
				std::swap(values[currentIndex.index], values.back());

				std::uint32_t temp = backIndex.index;
				backIndex.index = currentIndex.index;
				currentIndex.index = temp;
			}

			values.pop_back();
			currentIndex.generation++;
			if (currentIndex.generation == 0)
				currentIndex.generation = 1;
			freeIndices.push_back(std::distance(&*indexMapper.begin(), &currentIndex));
		}

		GenerationIndex& MapValueToIndex(const Ty& value)
		{
			auto index = &value - &*values.begin();
			return *std::ranges::find_if(indexMapper, [index](const GenerationIndex& i) { return i.index == index; });
		}

		const GenerationIndex& MapValueToIndex(const Ty& value) const
		{
			auto index = &value - &*values.begin();
			return *std::ranges::find_if(indexMapper, [index](const GenerationIndex& i) { return i.index == index; });
		}
	};
}