#pragma once

#include "SearchTimerCreateRequest.h"

#include <string>

// Versioned, immutable request evidence for SearchTimer CREATE.
// Share the exact provider option vocabulary rather than maintain an
// incomplete second list of fields in this durable operation type.
struct SearchTimerCreateOperationPayload : SearchTimerCreateRequest
{
    SearchTimerCreateOperationPayload() { compareTitle = true; }
};

bool searchTimerCreateOperationPayloadValid(
    const SearchTimerCreateOperationPayload& payload);

std::string serializeSearchTimerCreateOperationPayload(
    const SearchTimerCreateOperationPayload& payload);

bool parseSearchTimerCreateOperationPayload(
    const std::string& serialized,
    SearchTimerCreateOperationPayload& payload);

std::string searchTimerCreateOperationPayloadFingerprint(
    const SearchTimerCreateOperationPayload& payload);
