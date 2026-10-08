#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DevicePairingRequestRepository.h"
#include "DevicePairingRequestService.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityIdentityProvisioningRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
#include <sqlite3.h>

namespace
{
int countRows(Database& database, const char* sql)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.handle(), sql, -1,
        &statement, nullptr) != SQLITE_OK)
        return -1;
    const int count = sqlite3_step(statement) == SQLITE_ROW
        ? sqlite3_column_int(statement, 0) : -1;
    sqlite3_finalize(statement);
    return count;
}

DevicePairingRequestService::EntropySource deterministicEntropy()
{
    return [next = static_cast<unsigned char>(0x11)](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0U)
            return false;
        for (std::size_t index = 0U; index < size; ++index)
            output[index] = next++;
        return true;
    };
}

DevicePairingIssueRequest issueRequest(
    const std::string& displayName,
    const std::string& requestId)
{
    DevicePairingIssueRequest request;
    request.client.displayName = displayName;
    request.client.clientKind = "vidaa";
    request.client.appVersion = "1.0.0";
    request.requestId = requestId;
    request.correlationId =
        "mu10-correlation";
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository accountability(database);
    DevicePairingRequestRepository repository(database);
    assert(accountability.ensureSchema());
    assert(repository.ensureSchema());

    SecurityIdentityRepository identity(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    DeviceCredentialVerifierRepository deviceVerifiers(database);
    assert(identity.ensureSchema());
    assert(deviceVerifiers.ensureSchema());

    DevicePairingRequestService service(
        database,
        repository,
        accountability,
        deterministicEntropy(),
        []
        {
            return std::chrono::system_clock::time_point(
                std::chrono::seconds(4070908800));
        });

    DevicePairingIssueResult created =
        service.issue(issueRequest(
            "Living Room TV",
            "mu10b-create-approve"));
    assert(created.status == DevicePairingIssueStatus::issued);
    assert(created.pairing.has_value());
    const std::string approveId =
        created.pairing->resource.pairingRequestId;
    const std::string approveToken =
        created.pairing->pairingToken;

    const DevicePairingRequestLookupResult stored =
        repository.findById(approveId);
    assert(stored.status ==
        DevicePairingRequestRepositoryStatus::ok);
    assert(stored.request.revision == 1U);
    assert(stored.request.state == "pending");
    assert(stored.request.decidedByActorId.empty());
    assert(stored.request.decidedAt.empty());
    assert(stored.request.userCodeHash.rfind(
        "$6$rounds=10000$", 0U) == 0U);
    assert(stored.request.pairingTokenHash.rfind(
        "$6$rounds=10000$", 0U) == 0U);
    assert(stored.request.userCodeHash !=
        created.pairing->userCode);
    assert(stored.request.pairingTokenHash !=
        created.pairing->pairingToken);

    const DevicePairingAdministrationCollectionResult pendingList =
        service.listPendingForAdministration("", 50U);
    assert(pendingList.status ==
        DevicePairingAdministrationStatus::ok);
    assert(pendingList.requests.size() == 1U);
    assert(pendingList.requests.front().resource.state ==
        "pending");
    assert(pendingList.requests.front().resourceRevision ==
        "device-pairing:" + approveId + ":1");

    const DevicePairingAdministrationReadResult before =
        service.readForAdministration(approveId);
    assert(before.status ==
        DevicePairingAdministrationStatus::ok);
    assert(before.request.resourceRevision ==
        "device-pairing:" + approveId + ":1");

    DevicePairingDecisionRequest approve;
    approve.context.actorId = "admin-actor";
    approve.context.actorType = "user";
    approve.context.requestId = "mu10b-approve";
    approve.context.correlationId =
        "mu10-correlation";
    approve.pairingRequestId = approveId;
    approve.expectedResourceRevision =
        before.request.resourceRevision;
    approve.decision = "approve";

    const DevicePairingDecisionResult approved =
        service.decide(approve);
    assert(approved.status ==
        DevicePairingAdministrationStatus::ok);
    assert(approved.request.resource.state == "approved");
    assert(approved.request.resourceRevision ==
        "device-pairing:" + approveId + ":2");
    assert(approved.request.decidedByActorId ==
        "admin-actor");
    assert(!approved.request.decidedAt.empty());

    DevicePairingPollRequest poll;
    poll.pairingRequestId = approveId;
    poll.pairingToken = approveToken;
    const DevicePairingPollResult approvedPoll =
        service.poll(poll);
    assert(approvedPoll.status ==
        DevicePairingPollStatus::ok);
    assert(approvedPoll.resource.state == "approved");

    poll.pairingToken = "wrong-pairing-token";
    assert(service.poll(poll).status ==
        DevicePairingPollStatus::unauthorized);
    poll.pairingToken = approveToken;

    const DevicePairingAdministrationCollectionResult afterApproval =
        service.listPendingForAdministration("", 50U);
    assert(afterApproval.status ==
        DevicePairingAdministrationStatus::ok);
    assert(afterApproval.requests.empty());

    const DevicePairingDecisionResult stale =
        service.decide(approve);
    assert(stale.status ==
        DevicePairingAdministrationStatus::revisionConflict);

    DevicePairingDecisionRequest duplicate = approve;
    duplicate.expectedResourceRevision =
        approved.request.resourceRevision;
    const DevicePairingDecisionResult alreadyDecided =
        service.decide(duplicate);
    assert(alreadyDecided.status ==
        DevicePairingAdministrationStatus::stateConflict);

    // MU.10C: consumption is an atomic, revision-fenced operation and
    // must never occur implicitly merely because the TV polls.
    assert(repository.consumeApprovedInActiveTransaction(approveId, 2U) ==
        DevicePairingRequestRepositoryStatus::transactionRequired);
    assert(repository.consumeApprovedInActiveTransaction("", 2U) ==
        DevicePairingRequestRepositoryStatus::transactionRequired);
    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(repository.consumeApprovedInActiveTransaction(approveId, 2U) ==
        DevicePairingRequestRepositoryStatus::ok);
    assert(repository.findById(approveId).request.state == "consumed");
    assert(database.execute("ROLLBACK;"));
    assert(repository.findById(approveId).request.state == "approved");
    assert(repository.findById(approveId).request.revision == 2U);

    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(repository.consumeApprovedInActiveTransaction(approveId, 2U) ==
        DevicePairingRequestRepositoryStatus::ok);
    assert(database.execute("COMMIT;"));
    assert(repository.findById(approveId).request.revision == 3U);
    assert(repository.findById(approveId).request.state == "consumed");
    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(repository.consumeApprovedInActiveTransaction(approveId, 2U) ==
        DevicePairingRequestRepositoryStatus::revisionConflict);
    assert(repository.consumeApprovedInActiveTransaction(approveId, 3U) ==
        DevicePairingRequestRepositoryStatus::stateConflict);
    assert(database.execute("ROLLBACK;"));
    assert(service.poll(poll).status == DevicePairingPollStatus::unavailable);


    // Issue a fresh pairing and prove end-to-end canonical identity issuance.
    DevicePairingIssueResult deviceCreate =
        service.issue(issueRequest("Paired VIDAA TV", "mu10c-create"));
    assert(deviceCreate.status == DevicePairingIssueStatus::issued);
    assert(deviceCreate.pairing.has_value());
    const std::string issueId =
        deviceCreate.pairing->resource.pairingRequestId;
    DevicePairingCredentialIssueRequest deviceIssue;
    deviceIssue.pairingRequestId = issueId;
    deviceIssue.pairingToken = deviceCreate.pairing->pairingToken;
    deviceIssue.requestId = "mu10c-issue";
    deviceIssue.correlationId = "mu10c-correlation";
    assert(service.issueDeviceCredential(
        deviceIssue, provisioning, deviceVerifiers).status ==
        DevicePairingCredentialIssueStatus::notApproved);

    DevicePairingDecisionRequest deviceApprove = approve;
    deviceApprove.pairingRequestId = issueId;
    deviceApprove.expectedResourceRevision =
        "device-pairing:" + issueId + ":1";
    deviceApprove.context.requestId = "mu10c-approve";
    assert(service.decide(deviceApprove).status ==
        DevicePairingAdministrationStatus::ok);

    auto wrongIssue = deviceIssue;
    wrongIssue.pairingToken = "not-the-pairing-token";
    assert(service.issueDeviceCredential(
        wrongIssue, provisioning, deviceVerifiers).status ==
        DevicePairingCredentialIssueStatus::unauthorized);

    DevicePairingCredentialIssueResult credentialResult =
        service.issueDeviceCredential(
            deviceIssue, provisioning, deviceVerifiers);
    assert(credentialResult.status ==
        DevicePairingCredentialIssueStatus::issued);
    assert(credentialResult.credential.has_value());
    const auto& deviceCredential = *credentialResult.credential;
    assert(!deviceCredential.credentialSecret.empty());
    assert(deviceCredential.credentialSecret != deviceIssue.pairingToken);
    const auto actor = identity.findActor(deviceCredential.actorId);
    const auto device = identity.findDevice(deviceCredential.deviceId);
    const auto credential =
        identity.findCredential(deviceCredential.credentialId);
    const auto verifier = deviceVerifiers.findByCredentialId(
        deviceCredential.credentialId);
    assert(actor.has_value() && actor->type == ActorType::Service);
    assert(actor->actorId != "admin-actor");
    assert(device.has_value() &&
        device->actorId == deviceCredential.actorId);
    assert(credential.has_value() &&
        credential->actorId == deviceCredential.actorId &&
        credential->credentialType == "device-app");
    assert(verifier.has_value() &&
        verifier->deviceId == deviceCredential.deviceId &&
        verifier->verifierHash != deviceCredential.credentialSecret &&
        verifier->verifierHash.rfind("$6$", 0U) == 0U);
    assert(repository.findById(issueId).request.state == "consumed");
    assert(service.issueDeviceCredential(
        deviceIssue, provisioning, deviceVerifiers).status ==
        DevicePairingCredentialIssueStatus::consumed);
    credentialResult.credential->clearSecret();
    assert(credentialResult.credential->credentialSecret.empty());


    // Failure inside the issuer's write transaction must not leave
    // partially provisioned Actor/Device/Credential or consume approval.
    DevicePairingIssueResult failingCreate =
        service.issue(issueRequest("Rollback TV", "mu10c-rollback-create"));
    assert(failingCreate.status == DevicePairingIssueStatus::issued);
    assert(failingCreate.pairing.has_value());
    const std::string failingId =
        failingCreate.pairing->resource.pairingRequestId;
    DevicePairingDecisionRequest failingApprove = approve;
    failingApprove.pairingRequestId = failingId;
    failingApprove.expectedResourceRevision =
        "device-pairing:" + failingId + ":1";
    failingApprove.context.requestId = "mu10c-rollback-approve";
    assert(service.decide(failingApprove).status ==
        DevicePairingAdministrationStatus::ok);
    DevicePairingCredentialIssueRequest failingIssue;
    failingIssue.pairingRequestId = failingId;
    failingIssue.pairingToken = failingCreate.pairing->pairingToken;
    failingIssue.requestId = "mu10c-rollback-issue";
    const int actorsBefore = countRows(database,
        "SELECT COUNT(*) FROM security_actors;");
    const int devicesBefore = countRows(database,
        "SELECT COUNT(*) FROM security_devices;");
    const int credentialsBefore = countRows(database,
        "SELECT COUNT(*) FROM security_credentials;");
    assert(actorsBefore >= 0 && devicesBefore >= 0 &&
        credentialsBefore >= 0);
    assert(database.execute(
        "DROP TABLE security_device_credential_verifiers;"));
    assert(service.issueDeviceCredential(
        failingIssue, provisioning, deviceVerifiers).status ==
        DevicePairingCredentialIssueStatus::storageError);
    assert(repository.findById(failingId).request.state == "approved");
    assert(repository.findById(failingId).request.revision == 2U);
    assert(countRows(database, "SELECT COUNT(*) FROM security_actors;") ==
        actorsBefore);
    assert(countRows(database, "SELECT COUNT(*) FROM security_devices;") ==
        devicesBefore);
    assert(countRows(database, "SELECT COUNT(*) FROM security_credentials;") ==
        credentialsBefore);
    assert(deviceVerifiers.ensureSchema());
    assert(service.issueDeviceCredential(
        failingIssue, provisioning, deviceVerifiers).status ==
        DevicePairingCredentialIssueStatus::issued);
    assert(repository.findById(failingId).request.state == "consumed");

    DevicePairingIssueResult rejectedCreate =
        service.issue(issueRequest(
            "Bedroom TV",
            "mu10b-create-reject"));
    assert(rejectedCreate.status ==
        DevicePairingIssueStatus::issued);
    assert(rejectedCreate.pairing.has_value());

    const DevicePairingAdministrationReadResult rejectBefore =
        service.readForAdministration(
            rejectedCreate.pairing->resource.pairingRequestId);
    assert(rejectBefore.status ==
        DevicePairingAdministrationStatus::ok);

    DevicePairingDecisionRequest reject = approve;
    reject.context.requestId = "mu10b-reject";
    reject.pairingRequestId =
        rejectedCreate.pairing->resource.pairingRequestId;
    reject.expectedResourceRevision =
        rejectBefore.request.resourceRevision;
    reject.decision = "reject";
    const DevicePairingDecisionResult rejected =
        service.decide(reject);
    assert(rejected.status ==
        DevicePairingAdministrationStatus::ok);
    assert(rejected.request.resource.state == "rejected");
    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(repository.consumeApprovedInActiveTransaction(
        rejectedCreate.pairing->resource.pairingRequestId, 2U) ==
        DevicePairingRequestRepositoryStatus::stateConflict);
    assert(database.execute("ROLLBACK;"));

    DevicePairingPollRequest rejectedPoll;
    rejectedPoll.pairingRequestId =
        rejectedCreate.pairing->resource.pairingRequestId;
    rejectedPoll.pairingToken =
        rejectedCreate.pairing->pairingToken;
    assert(service.poll(rejectedPoll).status ==
        DevicePairingPollStatus::ok);
    assert(service.poll(rejectedPoll).resource.state ==
        "rejected");

    DevicePairingIssueResult expiringCreate =
        service.issue(issueRequest(
            "Expired TV",
            "mu10b-create-expired"));
    assert(expiringCreate.status ==
        DevicePairingIssueStatus::issued);
    assert(expiringCreate.pairing.has_value());

    const std::string expiredId =
        expiringCreate.pairing->resource.pairingRequestId;
    assert(database.execute(
        "UPDATE security_device_pairing_requests "
        "SET expires_at = '2000-01-01 00:00:00' "
        "WHERE pairing_request_id = '" +
        expiredId +
        "';"));

    DevicePairingDecisionRequest expired = approve;
    expired.context.requestId = "mu10b-expired";
    expired.pairingRequestId = expiredId;
    expired.expectedResourceRevision =
        "device-pairing:" + expiredId + ":1";
    assert(service.decide(expired).status ==
        DevicePairingAdministrationStatus::expired);
    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(repository.consumeApprovedInActiveTransaction(expiredId, 1U) ==
        DevicePairingRequestRepositoryStatus::expired);
    assert(database.execute("ROLLBACK;"));

    DevicePairingIssueRequest invalid =
        issueRequest("Invalid TV", "mu10b-invalid");
    invalid.client.clientKind = "not valid";
    assert(service.issue(invalid).status ==
        DevicePairingIssueStatus::invalidRequest);

    const std::vector<AccountabilityEvent> events =
        accountability.listAll();
    assert(std::any_of(
        events.begin(),
        events.end(),
        [&](const AccountabilityEvent& event)
        {
            return event.eventType ==
                    "device_pairing.administration" &&
                event.actorId == "admin-actor" &&
                event.permission == "device.pairing.decide" &&
                event.action == "device_pairing.approve" &&
                event.reasonCode ==
                    "pairing_request_approved" &&
                event.outcome == "success";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [&](const AccountabilityEvent& event)
        {
            return event.eventType ==
                    "device_pairing.administration" &&
                event.action == "device_pairing.reject" &&
                event.reasonCode ==
                    "pairing_request_rejected" &&
                event.outcome == "success";
        }));

    created.pairing->clearBootstrapMaterial();
    rejectedCreate.pairing->clearBootstrapMaterial();
    expiringCreate.pairing->clearBootstrapMaterial();
    assert(created.pairing->userCode.empty());
    assert(created.pairing->pairingToken.empty());

    DevicePairingRequestService entropyFailure(
        database,
        repository,
        accountability,
        [](unsigned char*, std::size_t)
        {
            return false;
        });
    assert(entropyFailure.issue(
        issueRequest("Entropy TV", "mu10b-entropy")).status ==
        DevicePairingIssueStatus::entropyUnavailable);

    return 0;
}
