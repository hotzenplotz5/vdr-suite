#pragma once

#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <sqlite3.h>

#include <string>

class SecurityIdentityProvisioningRepository
{
public:
    explicit SecurityIdentityProvisioningRepository(Database& database)
        : database_(database),
          identityRepository_(database)
    {
    }

    bool ensureIdentity(
        const std::string& actorId,
        ActorType actorType,
        const std::string& actorDisplayName,
        const std::string& deviceId,
        const std::string& deviceDisplayName,
        const std::string& sessionId,
        const std::string& credentialId,
        const std::string& credentialType)
    {
        if (actorId.empty() || actorDisplayName.empty() ||
            deviceId.empty() || deviceDisplayName.empty() ||
            sessionId.empty() || credentialId.empty() ||
            credentialType.empty())
        {
            return false;
        }

        if (!insertActorIfMissing(
                actorId,
                actorTypeName(actorType),
                actorDisplayName) ||
            !insertDeviceIfMissing(
                deviceId,
                actorId,
                deviceDisplayName) ||
            !insertSessionIfMissing(
                sessionId,
                actorId,
                deviceId) ||
            !insertCredentialIfMissing(
                credentialId,
                actorId,
                credentialType))
        {
            return false;
        }

        const auto actor = identityRepository_.findActor(actorId);
        const auto device = identityRepository_.findDevice(deviceId);
        const auto session = identityRepository_.findSession(sessionId);
        const auto credential =
            identityRepository_.findCredential(credentialId);

        return actor.has_value() &&
            actor->type == actorType &&
            actor->displayName == actorDisplayName &&
            device.has_value() &&
            device->actorId == actorId &&
            device->displayName == deviceDisplayName &&
            session.has_value() &&
            session->actorId == actorId &&
            session->deviceId == deviceId &&
            credential.has_value() &&
            credential->actorId == actorId &&
            credential->credentialType == credentialType;
    }

private:
    bool insertActorIfMissing(
        const std::string& actorId,
        const std::string& actorType,
        const std::string& displayName)
    {
        return executeThreeText(
            "INSERT OR IGNORE INTO security_actors "
            "(actor_id, actor_type, display_name) VALUES (?, ?, ?);",
            actorId,
            actorType,
            displayName);
    }

    bool insertDeviceIfMissing(
        const std::string& deviceId,
        const std::string& actorId,
        const std::string& displayName)
    {
        return executeThreeText(
            "INSERT OR IGNORE INTO security_devices "
            "(device_id, actor_id, display_name) VALUES (?, ?, ?);",
            deviceId,
            actorId,
            displayName);
    }

    bool insertSessionIfMissing(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& deviceId)
    {
        return executeThreeText(
            "INSERT OR IGNORE INTO security_sessions "
            "(session_id, actor_id, device_id) VALUES (?, ?, ?);",
            sessionId,
            actorId,
            deviceId);
    }

    bool insertCredentialIfMissing(
        const std::string& credentialId,
        const std::string& actorId,
        const std::string& credentialType)
    {
        return executeThreeText(
            "INSERT OR IGNORE INTO security_credentials "
            "(credential_id, actor_id, credential_type) VALUES (?, ?, ?);",
            credentialId,
            actorId,
            credentialType);
    }

    bool executeThreeText(
        const char* sql,
        const std::string& first,
        const std::string& second,
        const std::string& third)
    {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(
                database_.handle(),
                sql,
                -1,
                &statement,
                nullptr) != SQLITE_OK)
        {
            return false;
        }

        const bool bound =
            bindText(statement, 1, first) &&
            bindText(statement, 2, second) &&
            bindText(statement, 3, third);
        const int result = bound
            ? sqlite3_step(statement)
            : SQLITE_ERROR;
        sqlite3_finalize(statement);
        return result == SQLITE_DONE;
    }

    static bool bindText(
        sqlite3_stmt* statement,
        int index,
        const std::string& value)
    {
        return sqlite3_bind_text(
                   statement,
                   index,
                   value.c_str(),
                   -1,
                   SQLITE_TRANSIENT) == SQLITE_OK;
    }

    Database& database_;
    SecurityIdentityRepository identityRepository_;
};
