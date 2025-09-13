#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>
module InsanityEngine.Timer;
import InsanityEngine.Container.StableVector;

namespace InsanityEngine
{
	struct Timer
	{
		//std::string name;
		std::chrono::nanoseconds duration;
		std::chrono::nanoseconds elapsed{ 0 };
		//std::function<void()> onComplete;
		//std::function<void()> onPaused;


		std::chrono::nanoseconds ReverseElapsed() const noexcept { return duration - elapsed; }
		float ElapsedNormalized() const noexcept { return std::chrono::duration<float>{ elapsed } / std::chrono::duration<float>{ duration }; }
		float ReverseElapsedNormalized() const noexcept { return std::chrono::duration<float>{ elapsed } / std::chrono::duration<float>{ duration }; }
		bool Completed() const noexcept { return elapsed >= duration; }
	};

	struct TimerMetaData
	{
		std::string name;
		std::function<void()> onComplete;
		std::function<void()> onPaused;
	};

	static StableVector<Timer> timers;
	static StableVector<TimerMetaData> timerMetaData;
	static std::unordered_map<GenerationHandle, std::pair<Timer, TimerMetaData>> pausedTimers;

	static Timer& GetTimer(GenerationHandle handle)
	{
		if (Timer* t = timers.HandleAt(handle); t)
			return *t;

		return pausedTimers.at(handle).first;
	}

	static TimerMetaData& GetTimerMetaData(GenerationHandle handle)
	{
		if (TimerMetaData* t = timerMetaData.HandleAt(handle); t)
			return *t;

		return pausedTimers.at(handle).second;
	}

	UniqueTimerHandle NewTimer(std::string name, std::chrono::nanoseconds duration, std::function<void()> onComplete, std::function<void()> onPaused, std::chrono::nanoseconds startingElapsed)
	{
		auto& t = timers;
		timerMetaData.PushBack({ std::move(name), std::move(onComplete), std::move(onPaused) });
		return TimerHandle{ timers.PushBack({ duration, startingElapsed }) };
	}

	UniqueTimerHandle NewPausedTimer(std::string name, std::chrono::nanoseconds duration, std::function<void()> onComplete, std::function<void()> onPaused, std::chrono::nanoseconds startingElapsed)
	{
		UniqueTimerHandle t = NewTimer(std::move(name), duration, std::move(onComplete), std::move(onPaused), startingElapsed);

		GenerationHandle handle = t.Get().Get();
		auto pair = pausedTimers.insert({ handle, { std::move(*timers.HandleAt(handle)), std::move(*timerMetaData.HandleAt(handle)) } });
		timers.EraseHandle(handle);
		timerMetaData.EraseHandle(handle);

		return t;
	}

	void DeleteTimer(TimerHandle handle)
	{
		auto& t = timers;
		timers.EraseHandle(handle.Get());
		timerMetaData.EraseHandle(handle.Get());
		pausedTimers.erase(handle.Get());
	}

	void UpdateTimers(std::chrono::nanoseconds delta)
	{
		auto& t = timers;
		for (Timer& timer : timers)
		{
			timer.elapsed += delta;
		}

		std::size_t size = timers.Size();

		for (std::size_t i = 0; i < size;)
		{
			Timer& timer = timers.At(i);

			if (timer.Completed())
			{
				auto pair = pausedTimers.insert({ timers.MakeHandle(i), { std::move(timers.At(i)), std::move(timerMetaData.At(i)) } });
				timers.Erase(i);
				timerMetaData.Erase(i);
				size--;

				TimerMetaData& t = pair.first->second.second;
				if (t.onComplete)
					t.onComplete();
			}
			else
			{
				i++;
			}
		}
	}

    UniqueTimerHandle::~UniqueTimerHandle()
    {
		DeleteTimer(handle);
    }

	TimerHandle::operator bool() const noexcept
	{
		return handle.generation != 0;
	}

	void TimerHandle::Pause()
	{
		if (!timers.IsValid(handle))
			return;

		auto pair = pausedTimers.insert({ handle, { std::move(*timers.HandleAt(handle)), std::move(*timerMetaData.HandleAt(handle)) } });
		timers.EraseHandle(handle);
		timerMetaData.EraseHandle(handle);
		
		TimerMetaData& t = pair.first->second.second;
		if (t.onPaused)
			t.onPaused();
	}

	void TimerHandle::Resume()
	{
		Timer& timer = GetTimer(handle);
		TimerMetaData& timerMetaData = GetTimerMetaData(handle);
		*this = NewTimer(std::move(timerMetaData.name), timer.duration, std::move(timerMetaData.onComplete), std::move(timerMetaData.onPaused), timer.elapsed).Release();
	}

	void TimerHandle::Restart()
	{
		Timer& timer = GetTimer(handle);
		TimerMetaData& timerMetaData = GetTimerMetaData(handle);
		*this = NewTimer(std::move(timerMetaData.name), timer.duration, std::move(timerMetaData.onComplete), std::move(timerMetaData.onPaused)).Release();
	}

	std::chrono::nanoseconds TimerHandle::Elapsed() const
	{
		return GetTimer(handle).elapsed;
	}

	std::chrono::nanoseconds TimerHandle::ReverseElapsed() const
	{
		return GetTimer(handle).ReverseElapsed();
	}

	std::chrono::nanoseconds TimerHandle::Duration() const
	{
		return GetTimer(handle).duration;
	}

	float TimerHandle::ElapsedNormalized() const
	{
		return GetTimer(handle).ElapsedNormalized();
	}

	float TimerHandle::ReverseElapsedNormalized() const
	{
		return GetTimer(handle).ReverseElapsedNormalized();
	}

	bool TimerHandle::Completed() const
	{
		return GetTimer(handle).Completed();
	}

	bool TimerHandle::Paused() const
	{
		return pausedTimers.contains(handle);
	}
}