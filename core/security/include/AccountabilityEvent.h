#pragma once

#include <string>

struct AccountabilityEvent
{
    std::string eventId;
    int schemaVersion = 1;
    std::string classes;
    std::string eventType;
    std::string severity;
    std::string occurredAt;
    std::string actorId;
    std::string actorType;
    std::string deviceId;
    std::string sessionId;
    std::string authenticationState;
    std::string permission;
    std::string backendId;
    std::string operationId;
    std::string requestId;
    std::string correlationId;
    std::string action;
    std::string decision;
    std::string reasonCode;
    std::string outcome;
};
