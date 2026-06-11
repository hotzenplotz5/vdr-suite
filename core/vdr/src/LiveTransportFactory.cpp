#include "LiveTransportFactory.h"

#include "TestLiveTransport.h"

#include <stdexcept>

std::unique_ptr<ILiveTransport> LiveTransportFactory::create(
    const std::string& mode)
{
    if (mode == "test") {
        return std::make_unique<TestLiveTransport>();
    }

    throw std::invalid_argument("Unsupported live transport mode: " + mode);
}
