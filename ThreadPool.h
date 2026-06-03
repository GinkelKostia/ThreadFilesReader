#pragma once
#include <condition_variable>
#include <thread>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <functional>

namespace ISXFileReader {
	class ThreadPool {
	public:
		ThreadPool();
		~ThreadPool();

		void AddTask(const std::function<void()>& task);
		void Idle();
	private:
		std::condition_variable m_cv;
		std::condition_variable m_cv_end;
		std::vector<std::thread> m_threads;
		std::queue<std::function<void()>> m_queue;
		std::mutex m_mutex;
		std::atomic<int> m_active{ 0 };
		std::atomic<bool> m_is_end{ false };
		void WorkerThread();
	};
}