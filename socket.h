#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class ServerSocket;  // forward declaration so Socket can friend it

class Socket {
    int fd_{ -1 };

    explicit Socket(int fd) noexcept;  // private: only ServerSocket can call this
    friend class ServerSocket;

public:
    Socket();
    ~Socket();
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    [[nodiscard]] int fd() const noexcept { return fd_; }
    explicit operator bool() const noexcept { return fd_ != -1; }
};

class ServerSocket {
    int fd_{ -1 };
public:
    explicit ServerSocket(int port);
    ~ServerSocket();
    ServerSocket(const ServerSocket&) = delete;
    ServerSocket& operator=(const ServerSocket&) = delete;
    ServerSocket(ServerSocket&&) = delete;
    ServerSocket& operator=(ServerSocket&&) = delete;

    [[nodiscard]] Socket accept() const;
    explicit operator bool() const noexcept { return fd_ != -1; }
};
