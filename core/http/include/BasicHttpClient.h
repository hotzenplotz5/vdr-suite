#pragma once

#include "IHttpClient.h"

#include <string>

class BasicHttpClient : public IHttpClient {
public:
    BasicHttpClient(std::string host, int port);

    HttpResponse execute(const HttpRequest& request) const override;

private:
    std::string host_;
    int port_;
};
