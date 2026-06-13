#include "socket.h"
#include "ThreadPool.h"
#include "handle.h"
#include <iostream>
#include <unordered_map>
#include <optional>
#include <charconv>
#include <vector>
#include <algorithm>
#include <cctype>


bool iequals(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) ==
               std::tolower(static_cast<unsigned char>(y));
    });
}

std::optional<HttpRequest> parse_request(std::string_view buf) {
	HttpRequest request;
	constexpr int max_content_len {1000000};
	

	if (buf.find("\r\n\r\n") == std::string::npos) return std::nullopt;

	// parse first line
	const size_t method_space = buf.find(' ', 0);
	if (method_space == std::string::npos || method_space == 0) return std::nullopt;

	
	const size_t path_start = method_space + 1;
	const size_t path_end = buf.find(' ', path_start);
	if (path_end == std::string::npos) return std::nullopt;
	
	size_t version_start = buf.find('/', path_end);
	if (version_start == std::string::npos) return std::nullopt;
	++version_start; // starts after HTTP/

	const size_t version_end = buf.find("\r\n", version_start);
	if (version_end == std::string::npos) return std::nullopt;

	// parse headers
	size_t header_start = version_end + 2;
	const size_t header_end = buf.find("\r\n\r\n", version_end);
	if (header_end == std::string::npos) return std::nullopt;

	std::vector<HeaderField> header_pairs;
	size_t content_length = 0;

	// parse headers
	if (header_end > version_end) {
		while (header_start < buf.size()) {
			size_t line_end = buf.find("\r\n", header_start);
			if (line_end == std::string::npos) return std::nullopt;
			if (line_end == header_start) break;

			size_t header_name_end = buf.find(':', header_start);
			if (header_name_end == std::string::npos) return std::nullopt;

			size_t header_value_start = header_name_end + 1 + (buf[header_name_end + 1] == ' ');

			if (iequals(std::string_view(buf.data() + header_start, header_name_end - header_start), "Content-Length")) { // See if checking len then string is optimization
				auto [ptr, ec] = std::from_chars(buf.data() + header_value_start, buf.data() + line_end, content_length);
				if (ec != std::errc{}) return std::nullopt;
			}
			// TODO: check for Transfer-Encoding; if CL and TE then error; else use TE
			header_pairs.push_back({header_start, (header_name_end - header_start), header_value_start, (line_end - header_value_start)});

			header_start = line_end + 2;
		}
	}
	
	if (content_length > max_content_len) return std::nullopt;

	// receive rest of body
	if (content_length > 0) {
		size_t body_start = header_end + 4;
		size_t body_already_received = buf.size() - body_start;
		if (content_length > body_already_received) return std::nullopt;

		request.body = {buf.data() + body_start, content_length};
	}
	request.method = {buf.data(), method_space};
	request.version = {buf.data() + version_start, (version_end - version_start)};
	request.path = {buf.data() + path_start, (path_end - path_start)};

	// get headers
	for (HeaderField& f : header_pairs) {
		std::string_view header_name(buf.data() + f.name_start, f.name_len);
		request.headers[header_name] = {buf.data() + f.value_start, f.value_len};
	}

	return request;
}

void handle_request(const HttpRequest& req) {
	std::cout << req.method << " " << req.version << " " << req.path << std::endl;
		for (const auto& [key, value] : req.headers) {
			std::cout << key << ": " << value << std::endl;
		}
		std::cout << req.body << std::endl;
}

void handle_client(const Socket& client) {
    std::string buf;
	buf.reserve(4096);
	char chunk[4096];
	while (true) {
		if (auto req = parse_request(buf)) { 
			// TODO: erase consumed from buffer for multi requests
			handle_request(*req);
			break;
		}
		ssize_t n = recv(client.fd(), chunk, sizeof(chunk), 0);
		if (n <= 0) break;
		buf.append(chunk, n);
	}
}