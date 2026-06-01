#pragma once

#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

class RestfulApiVdrAdapter : public IVdrAdapter {
public:
    RestfulApiVdrAdapter(VdrConfig config, IHttpClient& httpClient);

    VdrStatus getStatus() const override;

private:
    VdrConfig config_;
    IHttpClient& httpClient_;
};
