#include "HttpRequest.h"

#include <cassert>
#include <string>

int main()
{
    HttpRequest request;

    request.method = "POST";
    request.url = "/api/test";
    request.headers["Content-Type"] = "application/json";
    request.headers["Accept"] = "application/json";
    request.body = "{\"hello\":\"world\"}";

    assert(request.method == "POST");
    assert(request.url == "/api/test");
    assert(request.headers.at("Content-Type") == "application/json");
    assert(request.headers.at("Accept") == "application/json");
    assert(request.body == "{\"hello\":\"world\"}");

    return 0;
}
