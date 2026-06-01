#include "MockHttpClient.h"

MockHttpClient::MockHttpClient()
{
    response_.statusCode = 200;
}

HttpResponse MockHttpClient::execute(const HttpRequest& request)
{
    requests_.push_back(request);
    return response_;
}

void MockHttpClient::setResponse(const HttpResponse& response)
{
    response_ = response;
}

const HttpRequest& MockHttpClient::lastRequest() const
{
    return requests_.back();
}

const std::vector<HttpRequest>& MockHttpClient::requests() const
{
    return requests_;
}

std::size_t MockHttpClient::requestCount() const
{
    return requests_.size();
}
