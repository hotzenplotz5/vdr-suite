#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DevicePairingRequestRepository.h"
#include "DevicePairingRequestService.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace
{
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
