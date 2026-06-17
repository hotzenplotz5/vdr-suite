#pragma once

#include "BasicHttpClientConfig.h"
#include "IHttpClient.h"

class BasicHttpClient final : public IHttpClient
{
public:
    explicit BasicHttpClient(
        BasicHttpClientConfig config);

    const BasicHttpClientConfig& config() const;

    HttpResponse execute(
        const HttpRequest& request) const override;

private:
    BasicHttpClientConfig config_;
};
