#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrTimerConflict.h"

#include <cassert>
#include <string>

int main()
{
    VdrTimerConflictEntry entry;
    entry.timerIndex = 14;
    entry.percentage = 84;
    entry.concurrentTimerIndices = {11, 12, 14, 13};

    VdrTimerConflict conflict;
    conflict.raw = "1783260840:14|84|11#12#14#13";
    conflict.conflictTime = 1783260840;
    conflict.entries.push_back(entry);

    VdrTimerConflictReport report;
    report.source = "restfulapi-epgsearch";
    report.available = true;
    report.checkAdvised = false;
    report.count = 1;
    report.total = 1;
    report.conflicts.push_back(conflict);

    VdrSnapshotReadJsonSerializer serializer;
    const std::string json =
        serializer.serializeTimerConflictReport(report);

    assert(json.find(R"JSON("source":"restfulapi-epgsearch")JSON") != std::string::npos);
    assert(json.find(R"JSON("available":true)JSON") != std::string::npos);
    assert(json.find(R"JSON("checkAdvised":false)JSON") != std::string::npos);
    assert(json.find(R"JSON("count":1)JSON") != std::string::npos);
    assert(json.find(R"JSON("conflictTime":1783260840)JSON") != std::string::npos);
    assert(json.find(R"JSON("timerIndex":14)JSON") != std::string::npos);
    assert(json.find(R"JSON("concurrentTimerIndices":[11,12,14,13])JSON") != std::string::npos);

    return 0;
}
