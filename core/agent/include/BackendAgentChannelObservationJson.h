#pragma once

#include <string>

struct BackendAgentObservationRequest;

bool parseBackendAgentChannelObservationJson(
    const std::string& body,
    BackendAgentObservationRequest& request,
    std::string& reasonCode);

std::string serializeBackendAgentChannelObservationJson(
    const BackendAgentObservationRequest& request);
