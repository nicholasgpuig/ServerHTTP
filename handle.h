#pragma once
#include <string>
#include <optional>
#include "http.h"
#include "socket.h"
#include "Router.h"

void handle_client(const Socket& client, Router& router);
std::optional<std::pair<HttpRequest, size_t>> parse_request(std::string_view buf);
std::string serialize_response(const HttpResponse& response);