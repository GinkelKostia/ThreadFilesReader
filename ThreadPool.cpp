#include "ThreadPool.h"

namespace ISXFileReader {
	ThreadPool::ThreadPool() {
		size_t thread_num = std::thread::hardware_concurrency();
		if (thread_num == 0) thread_num = 4;

		for (int i = 0; i < thread_num; i++) {
			m_threads.emplace_back(&ThreadPool::WorkerThread, this);
		}
	}

	ThreadPool::~ThreadPool() {
		m_is_end = true;
		m_cv.notify_all();
		for (std::thread& t : m_threads) {
			t.join();
		}
	}

	void ThreadPool::AddTask(const std::function<void()>& task) {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queue.push(task);
		}
		m_cv.notify_one();
	}

	void ThreadPool::Idle() {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv_end.wait(lock, [this]() { return m_queue.empty() && m_active.load() == 0; });
	}

	void ThreadPool::WorkerThread() {
		while (true) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_cv.wait(lock, [this]() { return !m_queue.empty() || m_is_end; });
				if (m_queue.empty() && m_is_end) return;
				task = m_queue.front();
				m_queue.pop();
			}
			m_active.fetch_add(1);

			task();

			m_active.fetch_sub(1);
			m_cv_end.notify_all();
		}
	}
}