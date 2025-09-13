module;

#include <chrono>
#include <string>
#include <functional>

export module InsanityEngine.Timer;
import InsanityEngine.Container.StableVector;

namespace InsanityEngine
{
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