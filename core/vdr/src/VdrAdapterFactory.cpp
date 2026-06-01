#include "VdrAdapterFactory.h"

#include <stdexcept>

#include "ExternalVdrAdapter.h"

std::unique_ptr<IVdrAdapter> VdrAdapterFactory::create(const VdrConfig& config)
{
    if (config.mode == "external") {
        return std::make_unique<ExternalVdrAdapter>(config);
    }

    throw std::invalid_argument("Unsupported VDR adapter mode: " + config.mode);
}
