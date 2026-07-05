#include "RestfulApiTimerConflictMapper.h"

#include <cassert>
#include <string>

int main()
{
    const std::string body =
        R"JSON({"check_advised":false,"conflicts":["1783260840:14|84|11#12#14#13","1783261500:13|33|11#12#14#13"],"count":2,"total":2})JSON";

    const VdrTimerConflictReport report =
        RestfulApiTimerConflictMapper::parseReport(body, 200);

    assert(report.source == "restfulapi-epgsearch");
    assert(report.available);
    assert(!report.checkAdvised);
    assert(report.count == 2);
    assert(report.total == 2);
    assert(report.conflicts.size() == 2);

    assert(report.conflicts[0].raw == "1783260840:14|84|11#12#14#13");
    assert(report.conflicts[0].conflictTime == 1783260840);
    assert(report.conflicts[0].entries.size() == 1);
    assert(report.conflicts[0].entries[0].timerIndex == 14);
    assert(report.conflicts[0].entries[0].percentage == 84);
    assert(report.conflicts[0].entries[0].concurrentTimerIndices.size() == 4);
    assert(report.conflicts[0].entries[0].concurrentTimerIndices[0] == 11);
    assert(report.conflicts[0].entries[0].concurrentTimerIndices[1] == 12);
    assert(report.conflicts[0].entries[0].concurrentTimerIndices[2] == 14);
    assert(report.conflicts[0].entries[0].concurrentTimerIndices[3] == 13);

    const VdrTimerConflictReport unavailable =
        RestfulApiTimerConflictMapper::parseReport("", 503);

    assert(!unavailable.available);
    assert(unavailable.error == "HTTP 503");

    return 0;
}
