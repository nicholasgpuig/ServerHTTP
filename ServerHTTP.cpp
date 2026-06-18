// ServerHTTP.cpp : Defines the entry point for the application.
//

#include "socket.h"
#include "Router.h"
#include "routes.h"
#include "EpollServer.h"

int main()
{
	constexpr int PORT = 8080;
	ServerSocket server = ServerSocket(PORT);
	if (!server) { return 1; }

	Router router;
	router.get("/hi", get_hi)
		.post("/hi", post_hi);

	EpollServer epoll_server(router);
	epoll_server.run(server);

	return 0;
}
