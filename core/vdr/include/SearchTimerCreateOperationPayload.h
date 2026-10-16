#pragma once

#include <string>

// Versioned, immutable request evidence for a future SearchTimer CREATE worker.
// This is NOT a VDR timer or a TimerAssignment and must never be dispatched
// without an independently verified backend write policy and readback.
struct SearchTimerCreateOperationPayload
{
    std::string name;
    std::string query;
    bool active = true;
    int matchMode = 0;
    int matchTolerance = 0;
    bool compareTitle = true;
    bool compareSubtitle = false;
    bool compareSummary = false;
};

bool searchTimerCreateOperationPayloadValid(
    const SearchTimerCreateOperationPayload& payload);

std::string serializeSearchTimerCreateOperationPayload(
    const SearchTimerCreateOperationPayload& payload);

bool parseSearchTimerCreateOperationPayload(
    const std::string& serialized,
    SearchTimerCreateOperationPayload& payload);

// A bounded, collision-resistant request fingerprint (sha256:<lowercase hex>).
std::string searchTimerCreateOperationPayloadFingerprint(
    const SearchTimerCreateOperationPayload& payload);
