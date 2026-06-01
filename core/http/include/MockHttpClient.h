#ifndef MOCK_HTTP_CLIENT_H
#define MOCK_HTTP_CLIENT_H

#include "IHttpClient.h"

#include <vector>

class MockHttpClient : public IHttpClient {
public:
    MockHttpClient();

    HttpResponse execute(const HttpRequest& request) override;

    void setResponse(const HttpResponse& response);

    const HttpRequest& lastRequest() const;
    const std::vector<HttpRequest>& requests() const;
    std::size_t requestCount() const;

private:
    HttpResponse response_;
    std::vector<HttpRequest> requests_;
};

#endif
