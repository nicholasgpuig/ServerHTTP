// ServerHTTP.cpp : Defines the entry point for the application.
//

#include "socket.h"
#include "ThreadPool.h"


int main()
{
	ServerSocket server = ServerSocket(8080);
	if (!server) { return 1; }

	ThreadPool<5> threadpool;
	while (true) {
		threadpool.add(server.accept());
	}

	// const char* response = "Hello\n";

    // send(client.fd(), response, static_cast<size_t>(7), 0);
	return 0;
}
