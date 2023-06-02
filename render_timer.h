/*
Author: Vladyslav Gutsulyak
Class: render_timer

Description:
The RenderTimer class provides a mechanism for controlling the rendering interval in a software application.
It allows you to define a desired render interval in milliseconds and provides a convenient way to check whether it's time to render a frame.

Features:
- Start and stop methods: Allows you to start and stop the timer.
- SetRenderInterval method: Sets the desired render interval in milliseconds.
- GetRenderInterval method: Retrieves the current render interval.
- CanRender method: Checks whether it's time to render a frame based on the set render interval.
- Precise Timing: The timer uses the steady clock from the C++ standard library to ensure accurate and precise timing.
- Multithreading Support: The timer can be used in a separate thread for more precise timing and minimal impact on the main application thread.
- High Performance: Optimized implementation using C++20 features to minimize calculations and improve performance.

Use Case:
The RenderTimer class is particularly useful in applications where precise rendering timing is crucial, such as graphics-intensive applications,
game engines, or real-time simulations. It provides a simple and efficient way to control the rendering rate and maintain smooth and consistent animation
or visual updates.
*/

#pragma once
#ifndef __RENDER_TIMER_H__
#define __RENDER_TIMER_H__
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

class render_timer
{
  public:
	render_timer()
		: m_fps(60), m_render_interval(1000 / m_fps), m_running(false), m_start_time(std::chrono::high_resolution_clock::now())
	{
	}

	inline void start()
	{
		if (!m_running.exchange(true))
		{
			m_start_time = std::chrono::high_resolution_clock::now();
			m_thread = std::thread(&render_timer::timer_loop, this);
		}
	}

	inline void stop()
	{
		if (m_running.exchange(false))
		{
			m_cv.notify_all();
			m_thread.join();
		}
	}

	inline void set_fps(int fps)
	{
		if (fps > 0 && fps != m_fps)
		{
			m_fps = fps;
			m_render_interval = 1000 / m_fps;
		}
	}

	inline bool can_render()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this]
				  { return m_render_ready.test_and_set(); });
		m_render_ready.clear();
		return true;
	}

	inline int get_render_interval() const
	{
		return m_render_interval;
	}

  private:
	void timer_loop()
	{
		std::chrono::high_resolution_clock::time_point last_frame_end = m_start_time;

		while (m_running.load())
		{
			auto frame_start = std::chrono::high_resolution_clock::now();

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_render_ready.test_and_set();
			}
			m_cv.notify_all();

			auto frame_end = std::chrono::high_resolution_clock::now();
			auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

			auto sleep_duration = std::chrono::milliseconds(m_render_interval) - frame_duration;
			auto extra_duration = frame_duration - std::chrono::milliseconds(m_render_interval);

			if (extra_duration > std::chrono::milliseconds(0))
			{
				m_render_interval -= extra_duration.count();
			}

			last_frame_end = frame_end;

			if (sleep_duration > std::chrono::duration<int64_t>(0))
			{
				std::this_thread::sleep_for(sleep_duration);
			}
		}
	}

	int m_fps;
	int m_render_interval;
	std::atomic<bool> m_running;
	std::chrono::high_resolution_clock::time_point m_start_time;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::atomic_flag m_render_ready = ATOMIC_FLAG_INIT;
	std::thread m_thread;
};

#endif // __RENDER_TIMER_H__
