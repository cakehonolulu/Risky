#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include "risky.h"

class SteppingThread {
public:
	SteppingThread() : running(false), updateFlag(false) {}

	~SteppingThread() {
		stop();
	}

	void start(const std::function<void()>& stepFunction) {
		if (running.exchange(true)) return;
		stepThread = std::thread([this, stepFunction]() {
			while (running) {
				stepFunction();
				updateFlag.store(true, std::memory_order_release);
				if (Risky::is_aborted()) break;
			}
			running = false;
		});
	}

	void stop() {
		running = false;
		if (stepThread.joinable()) {
			stepThread.join();
		}
	}

	bool isRunning() const {
		return running;
	}

	bool checkAndClearUpdateFlag() {
		return updateFlag.exchange(false, std::memory_order_acquire);
	}

private:
	std::thread stepThread;
	std::atomic<bool> running;
	std::atomic<bool> updateFlag;
};
