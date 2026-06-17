#pragma once

#include <string>

struct BasicHttpClientConfig
{
    std::string host = "127.0.0.1";
    int port = 80;
    int timeoutSeconds = 5;
};
