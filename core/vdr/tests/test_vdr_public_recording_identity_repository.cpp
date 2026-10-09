#include "Database.h"
#include "VdrPublicRecordingIdentityRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{
VdrRecording recording(const std::string& backend,
                       const std::string& native,
                       const std::string& start,
                       const std::string& title = "Film")
{
    VdrRecording value;
    value.backendId = backend;
    value.backendNativeId = native;
    value.startTime = start;
    value.title = title;
    return value;
}

void stableAcrossRefreshesAndRestart()
{
    const std::string file = "/tmp/vdr_public_recording_identity_test.db";
    std::remove(file.c_str());
    std::string stable;
    {
        Database database;
        assert(database.open(file));
        VdrPublicRecordingIdentityRepository repository(database);
        assert(repository.reconcileBackend("living-room", {
            recording("living-room", "/private/first.rec", "2026-10-09T20:00:00")
        }));
        auto listed = repository.activeBindingsForBackend("living-room");
        assert(listed.size() == 1);
        stable = listed.at(0).publicRecordingId;
        assert(stable.size() == 36 && stable.substr(0, 4) == "rec_");
        assert(stable.find("/private/") == std::string::npos);
        assert(repository.reconcileBackend("living-room", {
            recording("living-room", "/private/first.rec",
                "2026-10-09T20:00:00", "Renamed display title")
        }));
        listed = repository.activeBindingsForBackend("living-room");
        assert(listed.size() == 1);
        assert(listed.at(0).publicRecordingId == stable);
    }
    {
        Database database;
        assert(database.open(file));
        VdrPublicRecordingIdentityRepository repository(database);
        auto binding = repository.findActiveBinding(stable);
        assert(binding && binding->backendId == "living-room");
        assert(binding->sourceAddress == "/private/first.rec");
    }
    std::remove(file.c_str());
}

void backendIsolationAndDeletion()
{
    Database database;
    assert(database.open(":memory:"));
    VdrPublicRecordingIdentityRepository repository(database);
    assert(repository.reconcileBackend("A", {
        recording("A", "/same/path", "2026-10-09")
    }));
    assert(repository.reconcileBackend("B", {
        recording("B", "/same/path", "2026-10-09")
    }));
    const auto a = repository.activeBindingsForBackend("A");
    const auto b = repository.activeBindingsForBackend("B");
    assert(a.size() == 1 && b.size() == 1);
    assert(a[0].publicRecordingId != b[0].publicRecordingId);
    assert(repository.reconcileBackend("A", {}));
    assert(!repository.findActiveBinding(a[0].publicRecordingId));
    assert(repository.findActiveBinding(b[0].publicRecordingId));
    // A fresh appearance after confirmed removal must not silently revive
    // an old identity merely because the backend reused its path.
    assert(repository.reconcileBackend("A", {
        recording("A", "/same/path", "2026-10-09")
    }));
    assert(repository.activeBindingsForBackend("A")[0].publicRecordingId
        != a[0].publicRecordingId);
}

void refusesAmbiguousIdentityAndAtomicRollback()
{
    Database database;
    assert(database.open(":memory:"));
    VdrPublicRecordingIdentityRepository repository(database);
    assert(repository.reconcileBackend("A", {
        recording("A", "/old", "2026-10-09")
    }));
    const auto before = repository.activeBindingsForBackend("A");
    assert(!repository.reconcileBackend("A", {
        recording("A", "/new", "2026-10-10"),
        recording("A", "/new", "2026-10-10")
    }));
    assert(!repository.reconcileBackend("A", {
        recording("B", "/foreign", "2026-10-10")
    }));
    const auto after = repository.activeBindingsForBackend("A");
    assert(after.size() == 1 &&
           after[0].publicRecordingId == before[0].publicRecordingId);
    // Missing stable source evidence cannot create a public binding.
    assert(repository.reconcileBackend("A", {
        recording("A", "", "")
    }));
    assert(repository.activeBindingsForBackend("A").empty());
}

void verifiedMoveAndSourceReplacement()
{
    Database database;
    assert(database.open(":memory:"));
    VdrPublicRecordingIdentityRepository repository(database);
    assert(repository.reconcileBackend("A", {
        recording("A", "/old", "2026-10-09"),
        recording("A", "/occupied", "2026-10-09")
    }));
    const auto all = repository.activeBindingsForBackend("A");
    std::string target;
    for (const auto& binding : all)
        if (binding.sourceAddress == "/old") target = binding.publicRecordingId;
    assert(!target.empty());
    assert(!repository.rebindVerifiedMove(target, "B", "/old", "/new"));
    assert(!repository.rebindVerifiedMove(target, "A", "/incorrect", "/new"));
    assert(!repository.rebindVerifiedMove(target, "A", "/old", "/occupied"));
    assert(repository.rebindVerifiedMove(target, "A", "/old", "/new"));
    auto moved = repository.findActiveBinding(target);
    assert(moved && moved->sourceAddress == "/new");
    // Same path reused with another broadcast time receives a fresh ID.
    assert(repository.reconcileBackend("A", {
        recording("A", "/new", "2026-10-10")
    }));
    const auto replacement = repository.activeBindingsForBackend("A");
    assert(replacement.size() == 1);
    assert(replacement[0].publicRecordingId != target);
    assert(!repository.findActiveBinding(target));
}
}

int main()
{
    stableAcrossRefreshesAndRestart();
    backendIsolationAndDeletion();
    refusesAmbiguousIdentityAndAtomicRollback();
    verifiedMoveAndSourceReplacement();
    std::cout << "public Recording identity ledger: PASS" << std::endl;
}
