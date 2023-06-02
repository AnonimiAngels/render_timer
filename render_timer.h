#pragma once
#ifndef __RENDER_TIMER_H__
#define __RENDER_TIMER_H__
#include <chrono>
#include <condition_variable>
#include <thread>
#include <atomic>

class render_timer {
public:
    render_timer() : m_fps(60), m_render_interval(1000 / m_fps), m_running(false), m_start_time(std::chrono::high_resolution_clock::now()) {}

    void start()
    {
        if (!m_running.exchange(true)) {
            m_start_time = std::chrono::high_resolution_clock::now();
            m_thread = std::thread(&render_timer::timer_loop, this);
        }
    }

    void stop()
    {
        if (m_running.exchange(false)) {
            m_cv.notify_all();
            m_thread.join();
        }
    }

    void set_fps(int fps)
    {
        if (fps > 0 && fps != m_fps) {
            m_fps = fps;
            m_render_interval = 1000 / m_fps;
        }
    }

    bool can_render()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_render_ready.test_and_set(); });
        m_render_ready.clear();
        return true;
    }

    int get_render_interval() const
    {
        return m_render_interval;
    }

private:
    void timer_loop()
    {
        while (m_running.load()) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(m_render_interval));

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_render_ready.test_and_set();
            }
            m_cv.notify_all();

            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
            auto sleep_duration = std::chrono::milliseconds(m_render_interval) - frame_duration;

            if (sleep_duration > std::chrono::milliseconds(0)) {
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
