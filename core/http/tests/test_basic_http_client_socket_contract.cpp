#include "BasicHttpClient.h"

#include <cassert>
#include <stdexcept>
#include <string>

int main()
{
    BasicHttpClient client("127.0.0.1", 1);

    HttpRequest request;
    request.method = "GET";
    request.url = "/api/info.json";

    bool failedAsExpected = false;

    try
    {
        client.execute(request);
    }
    catch (const std::runtime_error& error)
    {
        const std::string message = error.what();
        failedAsExpected =
            message.find("connect failed to 127.0.0.1:1") != std::string::npos;
    }

    assert(failedAsExpected);

    return 0;
}
