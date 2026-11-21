#!/usr/bin/env python3
"""Run the exact native DELETE service body with isolated epgsearch test doubles.

This executes the patched C++ provider service implementation, not a Python
reimplementation. It is NOT a substitute for integration tests with a live
VDR, epgsearch threads, OSD menus, and a real cSafeFile filesystem.
"""
import argparse
import pathlib
import subprocess
import tempfile


def extract_service(source: str) -> str:
    start_marker = '    if (strcmp(Id, "Epgsearch-searchtimer-delete-cas-v1.0") == 0) {'
    end_marker = '    if (strcmp(Id, "Epgsearch-searchtimer-update-cas-v1.0") == 0) {'
    assert source.count(start_marker) == 1, "missing or ambiguous DELETE entry"
    start = source.index(start_marker)
    end = source.index(end_marker, start)
    body = source[start:end]
    assert "cMutexLock SearchExtsLock(&SearchExts);" in body
    assert "SearchExts.Del(rule, false);" in body, "freeing native OSD pointers is unsafe"
    assert "retiredNativeCasSearches.Add(rule);" in body
    assert "endcsOutcomeUnknown" in body
    assert "SearchExts.Save()" in body
    return body


def extract_clean_catalogue(source: str) -> str:
    begin = source.index("static bool NativeCasCatalogueClean()\n{")
    end = source.index("\n}\n\nbool cPluginEpgsearch::Service", begin) + 2
    return source[begin:end]


def extract_abi(source: str) -> str:
    start = source.index("enum eEpgsearchNativeDeleteCasStatus {")
    struct = source.index("struct Epgsearch_searchtimer_delete_cas_v1_0 {", start)
    end = source.index("};", struct) + 2
    return source[start:end]


HARNESS_HEADER = r"""
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct cSearchExt {
    int ID;
    std::string nativeGeneration;
    std::string nativeRevision;
    std::string lastPersistedText;
    std::string currentText;
    bool nativeGenerationDurable = true;
    bool nativeRevisionDurable = true;
    std::string ToText() const { return currentText; }
};
template<class T> struct cList {
    std::vector<T*> entries;
    void Add(T* entry) { entries.push_back(entry); }
};
struct cSearchExts {
    std::recursive_mutex mutex;
    std::vector<cSearchExt*> entries;
    std::atomic<int> saves{0};
    bool saveResult = true;
    cSearchExt* GetSearchFromID(int id) {
        for (auto* p : entries)
            if (p->ID == id) return p;
        return nullptr;
    }
    cSearchExt* First() { return entries.empty() ? nullptr : entries.front(); }
    cSearchExt* Next(cSearchExt* p) {
        auto pos = std::find(entries.begin(), entries.end(), p);
        if (pos == entries.end()) return nullptr;
        ++pos;
        return pos == entries.end() ? nullptr : *pos;
    }
    void Del(cSearchExt* rule, bool release) {
        assert(!release);
        auto it = std::find(entries.begin(), entries.end(), rule);
        assert(it != entries.end());
        entries.erase(it);
    }
    bool Save() {
        saves.fetch_add(1);
        return saveResult;
    }
} SearchExts;
struct cMutexLock {
    std::unique_lock<std::recursive_mutex> lock;
    explicit cMutexLock(cSearchExts *list) : lock(list->mutex) {}
};
struct {
    std::atomic<int> calls{0};
    void RemoveSearchID(int) { calls.fetch_add(1); }
} RecsDone;
struct {
    std::atomic<int> calls{0};
    void RemoveEntriesOfSearch(int) { calls.fetch_add(1); }
} TimersDone;
cList<cSearchExt> retiredNativeCasSearches;
"""

HARNESS_TESTS = r"""
static std::vector<std::unique_ptr<cSearchExt>> allocated;

static cSearchExt* add(int id, char gen, char rev, const char* text = "identical") {
    std::unique_ptr<cSearchExt> own(new cSearchExt);
    own->ID = id;
    own->nativeGeneration = std::string("sg_") + std::string(32, gen);
    own->nativeRevision = std::string("sr_") + std::string(32, rev);
    own->lastPersistedText = text;
    own->currentText = text;
    cSearchExt* p = own.get();
    allocated.push_back(std::move(own));
    SearchExts.entries.push_back(p);
    return p;
}
static void reset() {
    SearchExts.entries.clear();
    retiredNativeCasSearches.entries.clear();
    allocated.clear();
    SearchExts.saves.store(0);
    SearchExts.saveResult = true;
    RecsDone.calls.store(0);
    TimersDone.calls.store(0);
}
static Epgsearch_searchtimer_delete_cas_v1_0 request(int id, char gen, char rev) {
    Epgsearch_searchtimer_delete_cas_v1_0 out{};
    out.nativeId = id;
    out.expectedGeneration = std::string("sg_") + std::string(32, gen);
    out.expectedRevision = std::string("sr_") + std::string(32, rev);
    assert(Dispatch("Epgsearch-searchtimer-delete-cas-v1.0", &out));
    assert(out.atomicCompareAndWrite);
    return out;
}
int main() {
    // Service ABI capability check without a Data object.
    assert(Dispatch("Epgsearch-searchtimer-delete-cas-v1.0", nullptr));
    assert(!Dispatch("Epgsearch-searchtimer-no-cas-v1.0", nullptr));

    reset();
    cSearchExt* old = add(3, 'a', '1');
    auto staleGen = request(3, 'b', '1');
    assert(staleGen.status == endcsStaleGeneration && staleGen.noWriteAttested);
    auto staleRev = request(3, 'a', '2');
    assert(staleRev.status == endcsStaleRevision && staleRev.noWriteAttested);
    assert(SearchExts.saves == 0 && SearchExts.entries.size() == 1);

    old->currentText = "unpersisted";
    auto dirty = request(3, 'a', '1');
    assert(dirty.status == endcsUnpersistedChange && dirty.noWriteAttested);
    assert(SearchExts.saves == 0);
    old->currentText = "identical";
    old->nativeRevisionDurable = false;
    auto missingRevision = request(3, 'a', '1');
    assert(missingRevision.status == endcsUnavailable && missingRevision.noWriteAttested);
    old->nativeRevisionDurable = true;

    // The native cSafeFile writer rewrites every rule, not just target ID 3.
    // An unrelated dirty/incompletely migrated rule must fence the whole CAS.
    cSearchExt* other = add(13, 'e', '5');
    other->currentText = "unsaved legacy edit";
    auto otherDirty = request(3, 'a', '1');
    assert(otherDirty.status == endcsUnpersistedChange && otherDirty.noWriteAttested);
    assert(SearchExts.saves == 0 && SearchExts.entries.size() == 2);
    other->currentText = "identical";
    other->nativeGenerationDurable = false;
    auto otherNotDurable = request(3, 'a', '1');
    assert(otherNotDurable.status == endcsUnpersistedChange &&
           otherNotDurable.noWriteAttested);
    assert(SearchExts.saves == 0);
    other->nativeGenerationDurable = true;
    SearchExts.entries.pop_back();

    auto applied = request(3, 'a', '1');
    assert(applied.status == endcsApplied && !applied.noWriteAttested);
    assert(applied.confirmedDeletedGeneration == old->nativeGeneration);
    assert(SearchExts.entries.empty() && SearchExts.saves == 1);
    assert(RecsDone.calls == 1 && TimersDone.calls == 1);
    // Old OSD/search-thread pointer remains allocated after removing from list.
    assert(retiredNativeCasSearches.entries.back() == old);
    assert(old->ToText() == "identical");

    // Deleting and recreating the *same numeric ID and identical text*
    // must never let a previous public identifier delete the new rule.
    add(3, 'b', '1');
    auto reused = request(3, 'a', '1');
    assert(reused.status == endcsStaleGeneration && reused.noWriteAttested);
    assert(SearchExts.entries.size() == 1 && SearchExts.saves == 1);
    auto newDelete = request(3, 'b', '1');
    assert(newDelete.status == endcsApplied && SearchExts.saves == 2);
    auto notFound = request(3, 'b', '1');
    assert(notFound.status == endcsNotFound && notFound.noWriteAttested);
    assert(SearchExts.saves == 2);

    reset();
    add(4, 'c', '3');
    SearchExts.saveResult = false;
    auto unknown = request(4, 'c', '3');
    assert(unknown.status == endcsOutcomeUnknown && !unknown.noWriteAttested);
    assert(SearchExts.entries.empty() && SearchExts.saves == 1);
    assert(RecsDone.calls == 0 && TimersDone.calls == 0);
    auto noBlindRetry = request(4, 'c', '3');
    assert(noBlindRetry.status == endcsNotFound && SearchExts.saves == 1);

    // Simulate concurrent native CAS callers at the real extracted mutex
    // serialization point; exactly one can perform the deletion.
    reset();
    add(7, 'd', '4');
    Epgsearch_searchtimer_delete_cas_v1_0 x{}, y{};
    std::thread t1([&]{ x = request(7, 'd', '4'); });
    std::thread t2([&]{ y = request(7, 'd', '4'); });
    t1.join();
    t2.join();
    assert((x.status == endcsApplied && y.status == endcsNotFound) ||
           (y.status == endcsApplied && x.status == endcsNotFound));
    assert(SearchExts.saves == 1 && SearchExts.entries.empty());
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True,
                        help="epgsearch source with all native patches applied")
    args = parser.parse_args()
    cpp = (args.source / "epgsearch.c").read_text()
    header = (args.source / "services.h").read_text()
    body = extract_service(cpp)
    abi = extract_abi(header)
    clean_catalogue = extract_clean_catalogue(cpp)
    source = (HARNESS_HEADER + "\n" + abi + "\n" + clean_catalogue +
              "\nbool Dispatch(const char* Id, void* Data) {\n" + body +
              "    return false;\n}\n" + HARNESS_TESTS)
    with tempfile.TemporaryDirectory(prefix="s1-native-cas-") as dirname:
        work = pathlib.Path(dirname)
        (work / "harness.cpp").write_text(source)
        subprocess.run(["g++", "-std=c++11", "-pthread", "-Wall", "-Wextra",
                        "-Werror", str(work / "harness.cpp"), "-o",
                        str(work / "harness")], check=True)
        subprocess.run([str(work / "harness")], check=True)
    print("PASS: extracted real native DELETE-CAS body (stale ID/revision, "
          "duplicate ID+content, concurrent callers, dirty unrelated rules, "
          "failed save, retained pointers)")


if __name__ == "__main__":
    main()
