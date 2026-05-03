#pragma once

#include <condition_variable>
#include <mutex>

using namespace std;

template<typename T>
class EventPasser {

public:
	EventPasser(bool fake = false) : is_stopped(false), filled(false), fake(fake) {}
	
	EventPasser &operator=(EventPasser &&) = default;
	EventPasser(EventPasser &&) = default;

	
	~EventPasser() {
		stop();
	}

	void stop() {
		std::unique_lock l(mutex);
		is_stopped = true;

		l.unlock();
		cv.notify_all();
	}
	
	bool pop(T *const r) {
		if (fake)
		{
			*r = val;
			return true;
		}
		
		std::unique_lock l(mutex);
		cv.wait(l, [this] { return is_stopped || filled; });

		if (is_stopped) {
			return false;
		}

		*r = std::move(val);
		filled = false;

		return true;
	}
	
	template<class U = T>
	bool push(U &&x) {
		std::unique_lock l(mutex);
		const bool replaced = filled;
		val = std::forward<U>(x);
		filled = true;

		l.unlock();
		cv.notify_all();
		return replaced;
	}

private:
	std::mutex mutex;
	std::condition_variable cv;

	bool is_stopped;
	bool filled;
	bool fake;
	T val;

private:
	EventPasser(const EventPasser &other) = delete;
	EventPasser &operator=(const EventPasser &) = delete;
};