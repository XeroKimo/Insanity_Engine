module;

#include <cstdint>
#include <chrono>
#include <string>
#include <functional>
#include <vector>
#include <bit>
#include <utility>

export module InsanityEngine.Timer;

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

	constexpr GenerationHandle null_handle = { .generation = 0 };

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

	export class TimerHandle
	{
	private:
		GenerationHandle handle = null_handle;

	public:
		TimerHandle() = default;
		TimerHandle(std::nullptr_t) {}
		TimerHandle(GenerationHandle handle) : handle{ handle } {}
		TimerHandle(const TimerHandle&) = default;
		TimerHandle(TimerHandle&& other) noexcept :
			handle{ std::exchange(other.handle, null_handle) }
		{

		}


		TimerHandle& operator=(std::nullptr_t)
		{
			TimerHandle temp;
			swap(temp);
			return *this;
		}
		TimerHandle& operator=(const TimerHandle&) = default;
		TimerHandle& operator=(TimerHandle&& other) noexcept
		{
			TimerHandle temp{ std::move(other) };
			swap(temp);
			return *this;
		}

		bool operator==(const TimerHandle& other) const noexcept = default;

		bool operator==(std::nullptr_t) const noexcept
		{
			return handle == null_handle;
		}

		explicit operator bool() const noexcept;

	public:
		GenerationHandle Release() { return std::exchange(handle, null_handle); }
		GenerationHandle Get() const { return handle; }

		void Pause();
		void Resume();
		void Restart();

		std::chrono::nanoseconds Elapsed() const;
		std::chrono::nanoseconds ReverseElapsed() const;
		std::chrono::nanoseconds Duration() const;
		float ElapsedNormalized() const;
		float ReverseElapsedNormalized() const;
		bool Completed() const;
		bool Paused() const;

		void swap(TimerHandle& other) noexcept
		{
			std::swap(handle, other.handle);
		}
	};

	export class UniqueTimerHandle
	{
		TimerHandle handle;

	public:
		UniqueTimerHandle() = default;
		UniqueTimerHandle(std::nullptr_t) {}
		UniqueTimerHandle(TimerHandle handle) : handle{ handle } {}
		UniqueTimerHandle(const UniqueTimerHandle&) = delete;
		UniqueTimerHandle(UniqueTimerHandle&& other) noexcept :
			handle{ std::exchange(other.handle, null_handle) }
		{

		}

		~UniqueTimerHandle();

		UniqueTimerHandle& operator=(std::nullptr_t)
		{
			UniqueTimerHandle temp;
			handle.swap(temp.handle);
			return *this;
		}
		UniqueTimerHandle& operator=(const UniqueTimerHandle&) = delete;
		UniqueTimerHandle& operator=(UniqueTimerHandle&& other) noexcept
		{
			UniqueTimerHandle temp{ std::move(other) };
			handle.swap(temp.handle);
			return *this;
		}

		bool operator==(const TimerHandle& other) const noexcept
		{
			return handle == other;
		}

		bool operator==(std::nullptr_t) const noexcept
		{
			return handle == nullptr;
		}

		explicit operator bool() const noexcept { return handle.operator bool(); }

	public:
		TimerHandle Release() { return std::exchange(handle, {}); }
		TimerHandle Get() const noexcept { return handle; }

		void Pause() { return handle.Pause(); }
		void Resume() { return handle.Resume(); }
		void Restart() { return handle.Restart(); }

		std::chrono::nanoseconds Elapsed() const { return handle.Elapsed(); }
		std::chrono::nanoseconds ReverseElapsed() const { return handle.ReverseElapsed(); }
		std::chrono::nanoseconds Duration() const { return handle.Duration(); }
		float ElapsedNormalized() const { return handle.ElapsedNormalized(); }
		float ReverseElapsedNormalized() const { return handle.ReverseElapsedNormalized(); }
		bool Completed() const { return handle.Completed(); }
		bool Paused() const { return handle.Paused(); }
	};

	export UniqueTimerHandle NewTimer(std::string name, std::chrono::nanoseconds duration, std::function<void()> onComplete = {}, std::function<void()> onStopped = {}, std::chrono::nanoseconds startingElapsed = std::chrono::nanoseconds{ 0 });
	export UniqueTimerHandle NewPausedTimer(std::string name, std::chrono::nanoseconds duration, std::function<void()> onComplete = {}, std::function<void()> onStopped = {}, std::chrono::nanoseconds startingElapsed = std::chrono::nanoseconds{ 0 });
	export void DeleteTimer(TimerHandle handle);
	export void UpdateTimers(std::chrono::nanoseconds delta);
}

namespace std
{
	template<>
	class hash<InsanityEngine::GenerationHandle>
	{

	public:
		std::uint64_t operator()(const InsanityEngine::GenerationHandle& handle) const
		{
			return std::hash<std::uint32_t>{}(std::bit_cast<std::uint32_t>(handle));
		}
	};
}