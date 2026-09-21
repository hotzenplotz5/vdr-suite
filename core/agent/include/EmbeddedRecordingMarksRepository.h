#pragma once
#include "Database.h"
#include <string>

struct EmbeddedRecordingMarksRecord
{
    bool found = false;
    std::string backendId, operationId, requestIdentity, commandId, fingerprint;
    std::string payload, instanceId, state, evidence, canonicalRevision;
};

class EmbeddedRecordingMarksRepository
{
public:
    explicit EmbeddedRecordingMarksRepository(Database& database) : database_(database) {}
    bool ensureSchema();
    bool find(const std::string& backendId, const std::string& operationId,
        EmbeddedRecordingMarksRecord& record);
    bool insert(const EmbeddedRecordingMarksRecord& record);
    bool update(const EmbeddedRecordingMarksRecord& record);
private:
    Database& database_;
};
