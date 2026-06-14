// ServerHTTP.cpp : Defines the entry point for the application.
//

#include "socket.h"
#include "ThreadPool.h"
#include "Router.h"
#include "routes.h"

int main()
{
	constexpr int NUM_THREADS = 5;
	ServerSocket server = ServerSocket(8080);
	if (!server) { return 1; }

	Router router;
	router.get("/hi", get_hi)
		.post("/hi", post_hi);

	ThreadPool<NUM_THREADS> threadpool(router);
	while (true) {
		threadpool.add(server.accept());
	}

	return 0;
}
