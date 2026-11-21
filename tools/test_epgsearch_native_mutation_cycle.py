#!/usr/bin/env python3
"""Compile and exercise the actual epgsearch native CAS handler branches.

The epgsearch Service() UPDATE/TOGGLE/DELETE implementation and versioned C++
ABI structs are extracted from the fully patched upstream files. The mock
settings parser, SearchExts list, filesystem save, and scheduler are simplified.
This is neither a real config persistence test nor live VDR/OSD acceptance.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def native_service(source):
    start = '    if (strcmp(Id, "Epgsearch-searchtimer-delete-cas-v1.0") == 0) {'
    finish = '    if (strcmp(Id, "Epgsearch-searchtimer-observe-v1.1") == 0) {'
    assert source.count(start) == 1 and source.count(finish) == 1
    text = source[source.index(start):source.index(finish)]
    assert all(token in text for token in (
        "SearchExts.Del(rule, false)", "NativeCasCatalogueClean()",
        "enucsOutcomeUnknown", "entcsOutcomeUnknown", "endcsOutcomeUnknown",
        "SearchExts.Save()"))
    return text


def helper(source):
    marker = "static bool NativeCasCatalogueClean()\n{"
    start = source.index(marker)
    end = source.index("\n}\n\nbool cPluginEpgsearch::Service", start) + 2
    return source[start:end]


def native_abi(source):
    start = source.index("enum eEpgsearchNativeToggleCasStatus {")
    end = source.index('// Data structures for service "Epgsearch-services-v1.0"', start)
    text = source[start:end]
    for token in ("struct Epgsearch_searchtimer_toggle_cas_v1_0",
                  "struct Epgsearch_searchtimer_update_cas_v1_0",
                  "struct Epgsearch_searchtimer_delete_cas_v1_0"):
        assert token in text
    return text


CXX_HEADER = r"""
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
    int ID = -1;
    int useAsSearchTimer = 0;
    std::string title;
    std::string nativeGeneration;
    std::string nativeRevision;
    std::string lastPersistedText;
    bool nativeGenerationDurable = true;
    bool nativeRevisionDurable = true;
    cSearchExt& operator=(const cSearchExt& other) {
        // Match the actual epgsearch operator=: copy editable rule fields,
        // not generation/revision identity or persistence metadata.
        ID = other.ID;
        useAsSearchTimer = other.useAsSearchTimer;
        title = other.title;
        return *this;
    }
    bool Parse(const char* candidate) {
        const std::string src = candidate ? candidate : "";
        size_t a = src.find('|');
        size_t b = (a == std::string::npos) ? a : src.find('|', a + 1);
        if (a == std::string::npos || b == std::string::npos ||
            a == 0 || b == a + 1 || b + 1 >= src.size())
            return false;
        for (size_t i = 0; i < a; ++i)
            if (src[i] < '0' || src[i] > '9') return false;
        if (src.substr(a + 1, b - a - 1) != "0" &&
            src.substr(a + 1, b - a - 1) != "1")
            return false;
        ID = std::stoi(src.substr(0, a));
        useAsSearchTimer = src[a + 1] == '1' ? 1 : 0;
        title = src.substr(b + 1);
        return true;
    }
    std::string ToText() const {
        return std::to_string(ID) + "|" + std::to_string(useAsSearchTimer) +
               "|" + title;
    }
};
template<class T> struct cList {
    std::vector<T*> entries;
    void Add(T* obj) { entries.push_back(obj); }
};
struct cSearchExts {
    std::recursive_mutex mutex;
    std::vector<cSearchExt*> entries;
    int saves = 0;
    bool saveResult = true;
    cSearchExt* First() { return entries.empty() ? nullptr : entries.front(); }
    cSearchExt* Next(cSearchExt* item) {
        auto p = std::find(entries.begin(), entries.end(), item);
        if (p == entries.end() || ++p == entries.end()) return nullptr;
        return *p;
    }
    cSearchExt* GetSearchFromID(int id) {
        for (auto* p : entries) if (p->ID == id) return p;
        return nullptr;
    }
    void Del(cSearchExt* ptr, bool deleteObject) {
        assert(!deleteObject);
        auto p = std::find(entries.begin(), entries.end(), ptr);
        assert(p != entries.end());
        entries.erase(p);
    }
    bool Save() {
        ++saves;
        if (!saveResult) return false;
        // Only a test double: genuine epgsearch mints cryptographic revisions.
        // Revision rotation here lets us test compare-and-swap sequencing.
        for (auto* p : entries) {
            if (p->lastPersistedText != p->ToText()) {
                p->nativeRevision = std::string("sr_") +
                    std::string(32, char('c' + (saves % 3)));
            }
            p->lastPersistedText = p->ToText();
            p->nativeGenerationDurable = true;
            p->nativeRevisionDurable = true;
        }
        return true;
    }
} SearchExts;
struct cMutexLock {
    std::unique_lock<std::recursive_mutex> lock;
    explicit cMutexLock(cSearchExts* p) : lock(p->mutex) {}
};
struct { std::atomic<int> calls{0}; void RemoveSearchID(int) { ++calls; } } RecsDone;
struct { std::atomic<int> calls{0}; void RemoveEntriesOfSearch(int) { ++calls; } } TimersDone;
cList<cSearchExt> retiredNativeCasSearches;
struct { bool useSearchTimers = true; } EPGSearchConfig;
struct cSearchTimerThread {
    static void Init(void*, bool) { assert(false && "should not start scheduler"); }
};
"""

TESTS = r"""
static std::vector<std::unique_ptr<cSearchExt>> owned;
static cPluginEpgsearch provider;
static cSearchExt* create(int id, char generation, char rev, const char* name) {
    std::unique_ptr<cSearchExt> item(new cSearchExt);
    item->ID = id;
    item->title = name;
    item->nativeGeneration = "sg_" + std::string(32, generation);
    item->nativeRevision = "sr_" + std::string(32, rev);
    item->lastPersistedText = item->ToText();
    auto* ptr = item.get();
    owned.push_back(std::move(item));
    SearchExts.entries.push_back(ptr);
    return ptr;
}
static void reset() {
    SearchExts.entries.clear();
    retiredNativeCasSearches.entries.clear();
    owned.clear();
    SearchExts.saves = 0;
    SearchExts.saveResult = true;
    RecsDone.calls = 0;
    TimersDone.calls = 0;
}
static Epgsearch_searchtimer_update_cas_v1_0 update(
        int id, const std::string& gen, const std::string& rev,
        const std::string& settings) {
    Epgsearch_searchtimer_update_cas_v1_0 result{};
    result.nativeId = id;
    result.expectedGeneration = gen;
    result.expectedRevision = rev;
    result.settings = settings;
    assert(provider.Service("Epgsearch-searchtimer-update-cas-v1.0", &result));
    return result;
}
static Epgsearch_searchtimer_toggle_cas_v1_0 toggle(
        int id, const std::string& gen, const std::string& rev, bool enabled) {
    Epgsearch_searchtimer_toggle_cas_v1_0 result{};
    result.nativeId = id;
    result.expectedGeneration = gen;
    result.expectedRevision = rev;
    result.activate = enabled;
    assert(provider.Service("Epgsearch-searchtimer-toggle-cas-v1.0", &result));
    return result;
}
static Epgsearch_searchtimer_delete_cas_v1_0 remove(
        int id, const std::string& gen, const std::string& rev) {
    Epgsearch_searchtimer_delete_cas_v1_0 result{};
    result.nativeId = id;
    result.expectedGeneration = gen;
    result.expectedRevision = rev;
    assert(provider.Service("Epgsearch-searchtimer-delete-cas-v1.0", &result));
    return result;
}
int main() {
    assert(provider.Service("Epgsearch-searchtimer-update-cas-v1.0", nullptr));
    assert(provider.Service("Epgsearch-searchtimer-toggle-cas-v1.0", nullptr));
    assert(provider.Service("Epgsearch-searchtimer-delete-cas-v1.0", nullptr));
    assert(!provider.Service("Epgsearch-unguarded-delete-v1.0", nullptr));

    reset();
    cSearchExt* one = create(12, 'a', 'b', "same");
    std::string gen = one->nativeGeneration, rev = one->nativeRevision;
    const auto invalid = update(12, gen, rev, "12|1|must-stay-inactive");
    assert(invalid.status == enucsInvalid && SearchExts.saves == 0);
    const auto invalidId = update(12, gen, rev, "13|0|wrong-id");
    assert(invalidId.status == enucsInvalid && SearchExts.saves == 0);
    const auto stale = update(12, "sg_" + std::string(32, 'f'), rev,
                              "12|0|edited");
    assert(stale.status == enucsStaleGeneration &&
           stale.noWriteAttested && SearchExts.saves == 0);

    const auto changed = update(12, gen, rev, "12|0|edited");
    assert(changed.status == enucsApplied && changed.atomicCompareAndWrite);
    assert(changed.confirmedGeneration == gen);
    assert(changed.resultingRevision != rev && one->title == "edited");
    assert(one->nativeGeneration == gen && SearchExts.saves == 1);
    const auto oldRevision = update(12, gen, rev, "12|0|edited-again");
    assert(oldRevision.status == enucsStaleRevision &&
           oldRevision.noWriteAttested && SearchExts.saves == 1);
    rev = changed.resultingRevision;

    const auto on = toggle(12, gen, rev, true);
    assert(on.status == entcsApplied && one->useAsSearchTimer == 1);
    assert(on.resultingRevision != rev && SearchExts.saves == 2);
    rev = on.resultingRevision;
    assert(update(12, gen, rev, "12|0|forbidden").status == enucsNotInactive);

    const auto off = toggle(12, gen, rev, false);
    assert(off.status == entcsApplied && one->useAsSearchTimer == 0);
    assert(off.resultingRevision != rev && SearchExts.saves == 3);
    rev = off.resultingRevision;

    // A staged edit in a *different* native rule cannot be written
    // accidentally by a CAS transaction on this target.
    auto* other = create(44, 'c', 'd', "other");
    other->title = "unsaved";
    auto dirtyUpdate = update(12, gen, rev, "12|0|newer");
    assert(dirtyUpdate.status == enucsUnpersistedChange &&
           dirtyUpdate.noWriteAttested && SearchExts.saves == 3);
    auto dirtyToggle = toggle(12, gen, rev, true);
    assert(dirtyToggle.status == entcsUnpersistedChange &&
           dirtyToggle.noWriteAttested && SearchExts.saves == 3);
    auto dirtyDelete = remove(12, gen, rev);
    assert(dirtyDelete.status == endcsUnpersistedChange &&
           dirtyDelete.noWriteAttested && SearchExts.saves == 3);
    other->title = "other";
    SearchExts.entries.pop_back();

    const auto delStale = remove(12, gen, "sr_" + std::string(32, 'f'));
    assert(delStale.status == endcsStaleRevision && SearchExts.saves == 3);
    const auto del = remove(12, gen, rev);
    assert(del.status == endcsApplied && del.confirmedDeletedGeneration == gen);
    assert(SearchExts.saves == 4 && retiredNativeCasSearches.entries.back() == one);
    create(12, 'e', 'b', "edited"); // same numeric ID and matching content
    assert(remove(12, gen, rev).status == endcsStaleGeneration &&
           SearchExts.saves == 4);

    reset();
    auto* failed = create(20, 'a', 'b', "original");
    SearchExts.saveResult = false;
    auto unknown = update(20, failed->nativeGeneration, failed->nativeRevision,
                          "20|0|new-title");
    assert(unknown.status == enucsOutcomeUnknown && !unknown.noWriteAttested);
    assert(failed->title == "original" && !failed->nativeRevisionDurable);
    assert(SearchExts.saves == 1);

    reset();
    auto* race = create(21, 'a', 'b', "original");
    gen = race->nativeGeneration;
    rev = race->nativeRevision;
    Epgsearch_searchtimer_toggle_cas_v1_0 x{}, y{};
    std::thread t1([&]{ x = toggle(21, gen, rev, true); });
    std::thread t2([&]{ y = toggle(21, gen, rev, true); });
    t1.join();
    t2.join();
    assert((x.status == entcsApplied && y.status == entcsStaleRevision) ||
           (y.status == entcsApplied && x.status == entcsStaleRevision));
    assert(SearchExts.saves == 1 && race->useAsSearchTimer == 1);
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    args = parser.parse_args()
    source = (args.source / "epgsearch.c").read_text()
    service_abi = (args.source / "services.h").read_text()
    unit = (CXX_HEADER + "\n" + native_abi(service_abi) + "\n" +
            helper(source) + "\nclass cPluginEpgsearch {\npublic:\n"
            "bool Service(const char* Id, void* Data) {\n" +
            native_service(source) + "return false;\n}\n};\n" + TESTS)
    with tempfile.TemporaryDirectory(prefix="s1-native-cas-cycle-") as d:
        path = Path(d)
        (path / "cycle.cpp").write_text(unit)
        subprocess.run(["g++", "-std=c++11", "-pthread", "-Wall", "-Wextra",
                        "-Werror", str(path / "cycle.cpp"), "-o",
                        str(path / "cycle")], check=True)
        subprocess.run([str(path / "cycle")], check=True)
    print("PASS: extracted native UPDATE/TOGGLE/DELETE CAS branch cycle "
          "(mock parser/filesystem, isolated from VDR runtime)")


if __name__ == "__main__":
    main()
