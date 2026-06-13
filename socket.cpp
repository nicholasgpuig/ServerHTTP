#include "socket.h"
#include <utility>
#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>    // inet_pton(), inet_ntop()
#include <unistd.h>       // close(), read(), write()
#include <fcntl.h>        // fcntl(), O_NONBLOCK  (needed later for epoll)

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::Socket() noexcept {}

Socket::~Socket() {
	if (fd_ != -1) {
		close(fd_);
	}
}

Socket::Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
	if (this == &other) { return *this; }
	if (fd_ != -1) {
		close(fd_);
	}

	fd_ = std::exchange(other.fd_, -1);

	return *this;
}

// ServerSocket implementations

ServerSocket::ServerSocket(int port) {
	fd_ = socket(AF_INET, SOCK_STREAM, 0);

	if (fd_ == -1) { return; }

	int opt = 1;
	setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		close(fd_); fd_ = -1; return;
	}

	if (listen(fd_, SOMAXCONN) == -1) {
		close(fd_); fd_ = -1;
	}
}

ServerSocket::~ServerSocket() {
	if (fd_ != -1) { close(fd_); }
}

Socket ServerSocket::accept() const {
	int fd = ::accept(fd_, nullptr, nullptr);
	return Socket(fd);
}
