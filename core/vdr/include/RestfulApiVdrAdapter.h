#pragma once

#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrConfig.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class RestfulApiVdrAdapter : public IVdrAdapter {
public:
    RestfulApiVdrAdapter(VdrConfig config, IHttpClient& httpClient);

    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;
    std::vector<VdrChannel> getChannels() const override;
    std::vector<VdrTimer> getTimers() const override;
    std::vector<VdrRecording> getRecordings() const override;

private:
    VdrConfig config_;
    IHttpClient& httpClient_;
};
