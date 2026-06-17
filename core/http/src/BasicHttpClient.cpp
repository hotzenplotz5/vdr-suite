#include "BasicHttpClient.h"

BasicHttpClient::BasicHttpClient(
    BasicHttpClientConfig config)
    : config_(std::move(config))
{
}

const BasicHttpClientConfig& BasicHttpClient::config() const
{
    return config_;
}

HttpResponse BasicHttpClient::execute(
    const HttpRequest& request) const
{
    HttpResponse response;
    response.statusCode = 0;
    response.body =
        "BasicHttpClient network execution is not implemented yet for " +
        request.method + " " + request.url;
    return response;
}
