#include "HttpResponse.h"

#include <cassert>
#include <string>

int main()
{
    HttpResponse response;

    response.statusCode = 201;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\":\"created\"}";

    assert(response.statusCode == 201);
    assert(response.headers.at("Content-Type") == "application/json");
    assert(response.body == "{\"status\":\"created\"}");

    return 0;
}
