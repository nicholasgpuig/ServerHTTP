#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include "handle.h"
#include "Router.h"
#include "socket.h"
#include "EpollServer.h"

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

EpollServer::EpollServer(Router& router) : router_(router) {
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epfd_ == -1) { perror("epoll_create1"); exit(1); }
}

EpollServer::~EpollServer() {
    close(epfd_);
}

void EpollServer::run(ServerSocket& server) {
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.ptr = nullptr;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, server.fd(), &event);

    // wait for events
    epoll_event events[64];
    while (true) {
        int n = epoll_wait(epfd_, events, 64, -1);
        if (n == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.ptr == nullptr) {
                handle_new_connection(server);
            } else {
                int fd = static_cast<int>(reinterpret_cast<intptr_t>(events[i].data.ptr));
                handle_client_readable(fd);
            }
        }
    }
}

void EpollServer::handle_new_connection(ServerSocket& server) {
    Socket client = server.accept();
    if (!client) return;

    set_nonblocking(client.fd());

    int fd = client.fd();
    clients_.emplace(fd, ClientState(std::move(client)));

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.ptr = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &event);
}

void EpollServer::handle_client_readable(int fd) {
    auto it = clients_.find(fd);
    if (it == clients_.end()) return;

    ClientState& state = it->second;

    char chunk[4096];
    ssize_t n = recv(fd, chunk, sizeof(chunk), MSG_DONTWAIT);

    if (n > 0) {
        state.buf.append(chunk, n);
    } else if (n == -1 && errno == EAGAIN) {
        // No data right now, epoll will wake us again when there is
        return;
    } else {
        // Client closed (n == 0) or error (n == -1)
        remove_client(fd);
        return;
    }

    // try to parse and handle
    while (true) {
        if (auto req = parse_request(state.buf)) {
            auto [request, consumed] = *req;
            auto response = router_.dispatch(request);
            auto response_text = serialize_response(response);

            // Send response (non-blocking; may not send all bytes but assume it does for now)
            ssize_t sent = send(fd, response_text.data(), response_text.size(), MSG_DONTWAIT);
            if (sent == -1 && errno != EAGAIN) {
                remove_client(fd);
                return;
            }

            // erase consumed bytes for next request (keep-alive)
            state.buf.erase(0, consumed);
        } else {
            // incomplete request, wait for more data
            break;
        }
    }
}

void EpollServer::remove_client(int fd) {
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    clients_.erase(fd);
}