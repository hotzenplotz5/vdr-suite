#include "Database.h"
#include "MutationOperationRepository.h"
#include "SearchTimerCreateAdmissionService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>

using vdrsuite::operations::MutationOperationRepository;
using vdrsuite::operations::MutationOperationRepositoryStatus;

namespace
{
SearchTimerCreateAdmissionRequest request()
{
    SearchTimerCreateAdmissionRequest input;
    input.actorId = "actor:device-1";
    input.backendId = "default";
    input.backendGeneration = 3U;
    input.idempotencyKey = "searchtimer-create-unique-1";
    input.requestedAt = 1760000000;
    input.payload.name = "Terra X";
    input.payload.query = "Terra X";
    input.payload.matchMode = 1;
    input.payload.compareTitle = true;
    input.payload.compareSubtitle = false;
    input.payload.compareSummary = true;
    input.payload.marginStartMinutes = 2;
    input.payload.marginStopMinutes = 10;
    return input;
}

void assertPayloadCodec()
{
    auto p = request().payload;
    const std::string encoded =
        serializeSearchTimerCreateOperationPayload(p);
    assert(!encoded.empty());
    SearchTimerCreateOperationPayload decoded;
    assert(parseSearchTimerCreateOperationPayload(encoded, decoded));
    assert(decoded.name == p.name);
    assert(decoded.query == p.query);
    assert(decoded.matchMode == 1);
    assert(decoded.compareTitle && decoded.compareSummary);
    assert(decoded.marginStartMinutes == 2 && decoded.marginStopMinutes == 10);
    const std::string fingerprint =
        searchTimerCreateOperationPayloadFingerprint(p);
    assert(fingerprint.size() == 71U);
    assert(fingerprint.rfind("sha256:", 0U) == 0U);
    assert(!parseSearchTimerCreateOperationPayload(
        encoded + "ignored", decoded));
    assert(!parseSearchTimerCreateOperationPayload(
        "searchtimer-create-payload/2|", decoded));

    p.name = "Tatort | Zürich: Ausgabe";
    p.query = "Tatort";
    assert(parseSearchTimerCreateOperationPayload(
        serializeSearchTimerCreateOperationPayload(p), decoded));
    assert(decoded.name == p.name);

    p.marginStopMinutes = 1441;
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.marginStopMinutes = 10;
    p.marginStartMinutes = -1;
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.marginStartMinutes = 2;
    p.matchMode = 4; // Regexp explicitly not in Public v1 draft contract.
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.matchMode = 5;
    p.matchTolerance = 3;
    assert(searchTimerCreateOperationPayloadValid(p));
    p.compareTitle = false;
    p.compareSummary = false;
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.compareSubtitle = true;
    assert(searchTimerCreateOperationPayloadValid(p));
    p.query = std::string(241U, 'x');
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.query = std::string("\xc0\xaf", 2U); // overlong UTF-8
    assert(!searchTimerCreateOperationPayloadValid(p));
    p.query = "Tatort";
    p.name = " Tatort";
    assert(!searchTimerCreateOperationPayloadValid(p));
}

void assertDurableAdmissionAndReplay()
{
    Database db;
    assert(db.open(":memory:"));
    MutationOperationRepository store(db);
    assert(store.ensureSchema());
    SearchTimerCreateAdmissionService service(store);

    const auto first = service.admit(request());
    assert(first.status == SearchTimerCreateAdmissionStatus::prepared);
    assert(first.operation.operationRevision == "1");
    assert(first.operation.actionFamily == "searchtimer.create");
    assert(first.operation.resourceType == "SearchTimer");
    assert(first.operation.resourceId == "new");
    assert(first.operation.backendId == "default");
    assert(first.operation.backendGeneration == 3U);
    const auto stored =
        store.findPayloadByOperationId(first.operation.operationId);
    assert(stored.status == MutationOperationRepositoryStatus::ok);
    assert(stored.payload.payloadType == "searchtimer.create");
    assert(stored.payload.payloadVersion == 2U);
    assert(stored.payload.payloadFingerprint ==
        first.operation.requestFingerprint);

    // Restarting the admission service cannot repeat the reservation.
    SearchTimerCreateAdmissionService restarted(store);
    const auto again = restarted.admit(request());
    assert(again.status == SearchTimerCreateAdmissionStatus::replayed);
    assert(again.operation.operationId == first.operation.operationId);

    auto nextGeneration = request();
    nextGeneration.backendGeneration = 4U;
    const auto old = restarted.admit(nextGeneration);
    assert(old.status == SearchTimerCreateAdmissionStatus::replayed);
    assert(old.operation.backendGeneration == 3U);

    auto changed = request();
    changed.payload.compareSummary = false;
    const auto conflict = restarted.admit(changed);
    assert(conflict.status ==
        SearchTimerCreateAdmissionStatus::idempotencyConflict);
    assert(conflict.operation.operationId == first.operation.operationId);

    auto changedMargins = request();
    changedMargins.payload.marginStopMinutes = 11;
    assert(restarted.admit(changedMargins).status ==
        SearchTimerCreateAdmissionStatus::idempotencyConflict);

    auto changedName = request();
    changedName.payload.name = "Anderer Titel";
    assert(restarted.admit(changedName).status ==
        SearchTimerCreateAdmissionStatus::idempotencyConflict);

    auto independent = request();
    independent.idempotencyKey = "searchtimer-create-unique-2";
    const auto second = service.admit(independent);
    assert(second.status == SearchTimerCreateAdmissionStatus::prepared);
    assert(second.operation.operationId != first.operation.operationId);

    auto anotherActor = request();
    anotherActor.actorId = "actor:device-2";
    assert(service.admit(anotherActor).status ==
        SearchTimerCreateAdmissionStatus::prepared);
    auto anotherBackend = request();
    anotherBackend.backendId = "secondary";
    assert(service.admit(anotherBackend).status ==
        SearchTimerCreateAdmissionStatus::prepared);
}

void assertFileBackedRestartReplay()
{
    char filename[] = "/tmp/vdr-suite-searchtimer-admission-XXXXXX";
    const int fd = mkstemp(filename);
    assert(fd >= 0);
    assert(close(fd) == 0);
    std::string originalId;
    {
        Database db;
        assert(db.open(filename));
        MutationOperationRepository store(db);
        assert(store.ensureSchema());
        SearchTimerCreateAdmissionService service(store);
        const auto first = service.admit(request());
        assert(first.status == SearchTimerCreateAdmissionStatus::prepared);
        originalId = first.operation.operationId;
    }
    {
        Database db;
        assert(db.open(filename));
        MutationOperationRepository store(db);
        assert(store.ensureSchema());
        SearchTimerCreateAdmissionService service(store);
        const auto resumed = service.admit(request());
        assert(resumed.status == SearchTimerCreateAdmissionStatus::replayed);
        assert(resumed.operation.operationId == originalId);
        assert(store.findPayloadByOperationId(originalId).ok());
    }
    assert(std::remove(filename) == 0);
}

void assertFailClosedRequests()
{
    Database db;
    assert(db.open(":memory:"));
    MutationOperationRepository store(db);
    assert(store.ensureSchema());
    SearchTimerCreateAdmissionService service(store);
    auto invalid = request();
    invalid.backendGeneration = 0U;
    assert(service.admit(invalid).status ==
        SearchTimerCreateAdmissionStatus::invalid);
    invalid = request();
    invalid.backendId = "bad/backend";
    assert(service.admit(invalid).status ==
        SearchTimerCreateAdmissionStatus::invalid);
    invalid = request();
    invalid.idempotencyKey = "bad key";
    assert(service.admit(invalid).status ==
        SearchTimerCreateAdmissionStatus::invalid);
    invalid = request();
    invalid.payload.query = "";
    assert(service.admit(invalid).status ==
        SearchTimerCreateAdmissionStatus::invalid);
    invalid = request();
    invalid.payload.matchTolerance = 2;
    assert(service.admit(invalid).status ==
        SearchTimerCreateAdmissionStatus::invalid);

    // Without a repository schema there can be no successful admission,
    // and no provider write is ever attempted.
    Database unreadyDb;
    assert(unreadyDb.open(":memory:"));
    MutationOperationRepository unready(unreadyDb);
    SearchTimerCreateAdmissionService guarded(unready);
    assert(guarded.admit(request()).status ==
        SearchTimerCreateAdmissionStatus::repositoryUnavailable);
}
}

int main()
{
    assertPayloadCodec();
    assertDurableAdmissionAndReplay();
    assertFileBackedRestartReplay();
    assertFailClosedRequests();
    std::cout << "SearchTimer durable CREATE admission: PASS (no backend mutation)\n";
    return 0;
}
