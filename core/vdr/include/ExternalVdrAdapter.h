#pragma once

#include "IVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

class ExternalVdrAdapter : public IVdrAdapter {
public:
    explicit ExternalVdrAdapter(VdrConfig config);

    VdrStatus getStatus() const override;

private:
    VdrConfig config_;
};
