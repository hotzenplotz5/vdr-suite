#ifndef LIVE_TRANSPORT_FACTORY_H
#define LIVE_TRANSPORT_FACTORY_H

#include "ILiveTransport.h"

#include <memory>
#include <string>

class LiveTransportFactory {
public:
    static std::unique_ptr<ILiveTransport> create(
        const std::string& mode);
};

#endif
