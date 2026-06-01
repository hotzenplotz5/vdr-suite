#ifndef I_HTTP_CLIENT_H
#define I_HTTP_CLIENT_H

#include "HttpRequest.h"
#include "HttpResponse.h"

class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    virtual HttpResponse execute(const HttpRequest& request) = 0;
};

#endif
