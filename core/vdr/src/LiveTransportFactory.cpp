#include "LiveTransportFactory.h"

#include "TestLiveTransport.h"
#include "SseLiveTransport.h"

#include <stdexcept>

std::unique_ptr<ILiveTransport> LiveTransportFactory::create(
    const std::string& mode)
{
    if (mode == "test") {
        return std::make_unique<TestLiveTransport>();
    }

    if (mode == "sse") {
        return std::make_unique<SseLiveTransport>();
    }

    throw std::invalid_argument("Unsupported live transport mode: " + mode);
}
