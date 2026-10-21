#pragma once

#include "EpgTimerSelectionGuard.h"

#include <string>

class Database;

namespace vdrsuite::public_epg_timer
{
enum class AdmissionReplayStatus { absent, same, conflict, storageError };

struct AdmissionReplayResult
{
    AdmissionReplayStatus status = AdmissionReplayStatus::storageError;
    std::string operationId;
};

class EpgTimerIntentAdmissionRepository
{
public:
    explicit EpgTimerIntentAdmissionRepository(Database& database);
    bool ensureSchema();
    AdmissionReplayResult lookup(
        const std::string& authenticatedActorId, const Selection& selection);
    bool record(
        const std::string& authenticatedActorId, const Selection& selection,
        const std::string& intentId, const std::string& assignmentId,
        const std::string& operationId);
private:
    Database& database_;
};
}
