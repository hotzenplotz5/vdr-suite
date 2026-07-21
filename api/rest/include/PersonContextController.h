#pragma once

#include "DashboardController.h"

#include <string>

class PersonContextJsonSerializer;
class PersonContextService;

class PersonContextController
{
public:
    PersonContextController(
        PersonContextService& service,
        PersonContextJsonSerializer& serializer);

    ApiResponse getContext(
        const std::string& name,
        const std::string& providerPersonId,
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& fromTime,
        int limit,
        int offset) const;

private:
    PersonContextService& service_;
    PersonContextJsonSerializer& serializer_;
};
