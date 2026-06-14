#pragma once
#include <string>
#include "http.h"
#include "socket.h"
#include "Router.h"

void handle_client(const Socket& client, Router& router);
std::string serialize_response(const HttpResponse& response);