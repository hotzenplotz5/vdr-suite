#include "AccountabilityEventRepository.h"
#include "Database.h"

#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
AccountabilityEvent event()
{
    AccountabilityEvent value;
    value.eventId = "audit-1";
    value.classes = "audit";
    value.eventType = "authorization.allowed";
    value.severity = "info";
    value.occurredAt = "2026-07-27T12:00:00Z";
    value.actorId = "user-1";
    value.actorType = "user";
    value.deviceId = "device-1";
    value.sessionId = "session-1";
    value.authenticationState = "authenticated";
    value.permission = "remote.control";
    value.backendId = "default";
    value.operationId = "op-1";
    value.requestId = "req-1";
    value.correlationId = "corr-1";
    value.action = "remote.control";
    value.decision = "allowed";
    value.reasonCode = "permission_granted";
    value.outcome = "dispatch_authorized";
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.append(event()));

    std::ostringstream diagnostics;
    std::streambuf* previousErrorBuffer =
        std::cerr.rdbuf(diagnostics.rdbuf());
    assert(!repository.append(event()));
    std::cerr.rdbuf(previousErrorBuffer);

    const std::string diagnosticText = diagnostics.str();
    assert(
        diagnosticText.find(
            "accountability append step failed: sqlite_rc=") !=
        std::string::npos);
    assert(
        diagnosticText.find("sqlite_extended_rc=") !=
        std::string::npos);
    assert(diagnosticText.find("audit-1") == std::string::npos);
    assert(diagnosticText.find("user-1") == std::string::npos);

    const auto events = repository.listAll();
    assert(events.size() == 1);
    assert(events.front().eventId == "audit-1");
    assert(events.front().actorId == "user-1");
    assert(events.front().backendId == "default");
    assert(events.front().operationId == "op-1");
    assert(events.front().decision == "allowed");

    assert(!database.execute(
        "UPDATE accountability_events "
        "SET decision='denied' "
        "WHERE event_id='audit-1';"));
    assert(!database.execute(
        "DELETE FROM accountability_events "
        "WHERE event_id='audit-1';"));

    const auto preserved = repository.listAll();
    assert(preserved.size() == 1);
    assert(preserved.front().decision == "allowed");

    AccountabilityEvent invalid = event();
    invalid.eventId.clear();
    assert(!repository.append(invalid));

    AccountabilityEvent concurrent = event();
    concurrent.eventId = "audit-2";
    concurrent.requestId = "req-2";

    std::promise<void> appendStartedPromise;
    std::future<void> appendStarted = appendStartedPromise.get_future();
    std::future<bool> appendResult;

    {
        auto transactionLease = database.acquireTransactionLease();
        appendResult = std::async(
            std::launch::async,
            [&repository, &concurrent, &appendStartedPromise]()
            {
                appendStartedPromise.set_value();
                return repository.append(concurrent);
            });

        appendStarted.wait();
        assert(
            appendResult.wait_for(std::chrono::milliseconds(100)) ==
            std::future_status::timeout);
    }

    assert(appendResult.get());

    const auto serialized = repository.listAll();
    assert(serialized.size() == 2);
    assert(serialized.back().eventId == "audit-2");

    return 0;
}
