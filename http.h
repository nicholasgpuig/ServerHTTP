#pragma once
#include <string_view>
#include <unordered_map>
#include <string>

struct HttpRequest {
	std::string_view method;
	std::string_view path;
	std::string_view version;
	std::unordered_map<std::string_view, std::string_view> headers;
	std::string_view body;
};

struct HttpResponse {
	int status;
    std::unordered_map<std::string, std::string> headers;
	std::string body;
};

struct HeaderField {
	size_t name_start;
	size_t name_len;
	size_t value_start;
	size_t value_len;
};

inline HttpResponse create_not_found_response() {
    return {404, {{}}, "Not Found\n"};
}