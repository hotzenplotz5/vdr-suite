#pragma once

#include "Database.h"

#include <string>

struct EmbeddedRecordingCutRecord
{
    bool found = false;
    std::string backendId;
    std::string operationId;
    std::string requestIdentity;
    std::string commandId;
    std::string fingerprint;
    std::string payload;
    std::string instanceId;
    std::string state;
    std::string evidence;
    std::string editedRecordingKey;
};

class EmbeddedRecordingCutRepository
{
public:
    explicit EmbeddedRecordingCutRepository(Database& database)
        : database_(database)
    {
    }

    bool ensureSchema();

    bool find(
        const std::string& backendId,
        const std::string& operationId,
        EmbeddedRecordingCutRecord& record);

    bool insert(const EmbeddedRecordingCutRecord& record);
    bool update(const EmbeddedRecordingCutRecord& record);

private:
    Database& database_;
};
