#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include "http.h"

class Router {
using Handler = std::function<HttpResponse(const HttpRequest&)>;
private:
    std::unordered_map<std::string, Handler> routes;
public:
    Router& get(std::string path, Handler handler);
    Router& post(std::string path, Handler handler);
    HttpResponse dispatch(const HttpRequest& req);
};