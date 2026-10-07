#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DevicePairingRequestRepository.h"
#include "DevicePairingRequestService.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
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

    DevicePairingIssueRequest issue;
    issue.client.displayName = "Living Room TV";
    issue.client.clientKind = "vidaa";
    issue.client.appVersion = "1.0.0";
    issue.requestId = "mu10a-create-001";
    issue.correlationId = "mu10a-correlation-001";

    DevicePairingIssueResult created =
        service.issue(issue);
    assert(created.status == DevicePairingIssueStatus::issued);
    assert(created.pairing.has_value());
    assert(
        created.pairing->resource.pairingRequestId.rfind(
            "dpr_",
            0U) == 0U);
    assert(created.pairing->userCode.size() == 9U);
    assert(created.pairing->userCode[4] == '-');
    assert(created.pairing->pairingToken.size() >= 32U);
    assert(created.pairing->resource.state == "pending");
    assert(
        created.pairing->resource.pollIntervalSeconds ==
        DevicePairingRequestService::PollIntervalSeconds);
    assert(
        created.pairing->resource.expiresAt ==
        "2099-01-01T00:10:00Z");

    const DevicePairingRequestLookupResult stored =
        repository.findById(
            created.pairing->resource.pairingRequestId);
    assert(
        stored.status ==
        DevicePairingRequestRepositoryStatus::ok);
    assert(
        stored.request.userCodeHash !=
        created.pairing->userCode);
    assert(
        stored.request.pairingTokenHash !=
        created.pairing->pairingToken);
    assert(
        stored.request.userCodeHash.rfind(
            "$6$rounds=10000$",
            0U) == 0U);
    assert(
        stored.request.pairingTokenHash.rfind(
            "$6$rounds=10000$",
            0U) == 0U);

    DevicePairingPollRequest poll;
    poll.pairingRequestId =
        created.pairing->resource.pairingRequestId;
    poll.pairingToken =
        created.pairing->pairingToken;

    const DevicePairingPollResult pending =
        service.poll(poll);
    assert(pending.status == DevicePairingPollStatus::ok);
    assert(pending.resource.state == "pending");
    assert(
        pending.resource.client.displayName ==
        "Living Room TV");
    assert(
        pending.resource.client.clientKind ==
        "vidaa");
    assert(
        pending.resource.client.appVersion ==
        "1.0.0");

    poll.pairingToken =
        "wrong-pairing-token";
    assert(
        service.poll(poll).status ==
        DevicePairingPollStatus::unauthorized);

    poll.pairingRequestId =
        "dpr_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    assert(
        service.poll(poll).status ==
        DevicePairingPollStatus::notFound);

    poll.pairingRequestId =
        created.pairing->resource.pairingRequestId;
    poll.pairingToken =
        created.pairing->pairingToken;
    assert(database.execute(
        "UPDATE security_device_pairing_requests "
        "SET expires_at = '2000-01-01 00:00:00' "
        "WHERE pairing_request_id = '" +
        created.pairing->resource.pairingRequestId +
        "';"));
    assert(
        service.poll(poll).status ==
        DevicePairingPollStatus::expired);

    DevicePairingIssueRequest invalid = issue;
    invalid.client.clientKind = "not valid";
    assert(
        service.issue(invalid).status ==
        DevicePairingIssueStatus::invalidRequest);

    const std::vector<AccountabilityEvent> events =
        accountability.listAll();
    assert(events.size() == 1U);
    assert(
        events.front().eventType ==
        "device_pairing.requested");
    assert(
        events.front().actorId ==
        "anonymous");
    assert(
        events.front().permission ==
        "device.pairing.bootstrap");
    assert(
        events.front().operationId ==
        created.pairing->resource.pairingRequestId);
    assert(
        events.front().requestId ==
        "mu10a-create-001");

    created.pairing->clearBootstrapMaterial();
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
    assert(
        entropyFailure.issue(issue).status ==
        DevicePairingIssueStatus::entropyUnavailable);

    return 0;
}
