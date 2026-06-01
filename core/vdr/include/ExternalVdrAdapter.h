#pragma once

#include "IVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrEvent.h"
#include "VdrStatus.h"

#include <vector>

class ExternalVdrAdapter : public IVdrAdapter {
public:
    explicit ExternalVdrAdapter(VdrConfig config);

    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;

private:
    VdrConfig config_;
};
