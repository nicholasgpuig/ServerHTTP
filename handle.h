#pragma once
#include "socket.h"
#include <string_view>
#include <unordered_map>
#include <optional>

struct HttpRequest {
	std::string_view method;
	std::string_view path;
	std::string_view version;
	std::unordered_map<std::string_view, std::string_view> headers;
	std::string_view body;
};

struct HeaderField {
	size_t name_start;
	size_t name_len;
	size_t value_start;
	size_t value_len;
};

std::optional<HttpRequest> parse_request(std::string_view buf);
void handle_request(const HttpRequest& req);
void handle_client(const Socket& client);