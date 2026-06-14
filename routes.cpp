#include <string>
#include "http.h"

HttpResponse get_hi(const HttpRequest& req) {
	return {200, {{"Content-Type", "text/plain"}}, "Hi, nice to meet you.\n"};
}

HttpResponse post_hi(const HttpRequest& req) {
	HttpResponse res;
	res.status = 200;
	res.body = "Hi " + std::string{req.body} + ", nice to meet you.\n";
	res.headers["Content-Type"] = "text/plain";
	res.headers["Content-Length"] = std::to_string(res.body.size());
	return res;
}