#pragma once
#include <string>
#include <unordered_map>
#include "Router.h"
#include "socket.h"

struct ClientState {
    Socket socket;
    std::string buf;

    explicit ClientState(Socket s) : socket(std::move(s)) {
        buf.reserve(4096);
    }
};

class EpollServer {
private:
    int epfd_;
    Router& router_;
    std::unordered_map<int, ClientState> clients_;

public:
    explicit EpollServer(Router& router);
    ~EpollServer();

    void run(ServerSocket& sock);

private:
    void handle_new_connection(ServerSocket& socket);
    void handle_client_readable(int fd);
    void remove_client(int fd);
};