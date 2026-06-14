#include <string>
#include "http.h"
#include "Router.h"

using Handler = std::function<HttpResponse(const HttpRequest&)>;
Router& Router::get(std::string path, Handler handler) {
    routes["GET " + path] = handler;
    return *this;
}

Router& Router::post(std::string path, Handler handler) {
    routes["POST " + path] = handler;
    return *this;
}

HttpResponse Router::dispatch(const HttpRequest& req) {
    std::string path = std::string{req.method} + " " + std::string{req.path};
    if (routes.find(path) == routes.end()) {
        return create_not_found_response();
    }
    return routes[path](req);
}