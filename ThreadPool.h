#pragma once
#include "socket.h"
#include "handle.h"
#include "Router.h"
#include <array>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <int N>
class ThreadPool {
private:
    std::array<std::jthread, N> workers;
    std::queue<Socket> q;
    std::mutex m;
    std::condition_variable cv;
    Router& router;
    
public:

    ThreadPool(Router& router_) : router(router_) {
        for (int i = 0; i < N; ++i) {
            workers[i] = std::jthread([this](std::stop_token st) -> void {
                while (!st.stop_requested()) {
                    Socket sock;
                    {
                        std::unique_lock lock(m);
                        cv.wait(lock, [this, &st]() -> bool {
                            return !q.empty() || st.stop_requested();
                        });
                        if (!q.empty()) {
                            sock = std::move(q.front());
                            q.pop();
                        }
                    }
                    if (sock) {
                        handle_client(sock, router);
                    }
                }
            });
        }
    }
    ~ThreadPool() {}

    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;


    void add(Socket&& sock) {
        {
            std::lock_guard lock(m);
            q.push(std::move(sock));
        }
        cv.notify_one();
    }
};