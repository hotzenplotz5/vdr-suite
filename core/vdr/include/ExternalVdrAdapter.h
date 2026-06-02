#pragma once

#include "IVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

class ExternalVdrAdapter : public IVdrAdapter {
public:
    explicit ExternalVdrAdapter(VdrConfig config);

    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;
    std::vector<VdrChannel> getChannels() const override;
    std::vector<VdrRecording> getRecordings() const override;

private:
    VdrConfig config_;
};
