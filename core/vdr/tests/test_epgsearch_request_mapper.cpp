#include "EpgSearchRequest.h"
#include "EpgSearchRequestMapper.h"
#include "EpgSearchQuery.h"

#include <cassert>
#include <iostream>

int main()
{
    const EpgSearchRequestMapper mapper;

    const EpgSearchQuery emptyQuery =
        mapper.map(EpgSearchRequest::all());

    assert(!emptyQuery.hasText());
    assert(!emptyQuery.hasBackend());
    assert(emptyQuery.hasFieldSelection());
    assert(emptyQuery.useTitle());
    assert(emptyQuery.useSubtitle());
    assert(emptyQuery.useDescription());

    EpgSearchRequest request =
        EpgSearchRequest::sorted(
            "home-vdr",
            "Terra X",
            "C-1-1051-10301",
            1000,
            7200,
            50,
            10,
            EpgSearchSortField::StartTime,
            EpgSearchSortOrder::Descending);

    request.setSearchFields(
        true,
        false,
        true);

    const EpgSearchQuery query =
        mapper.map(request);

    assert(!query.isEmpty());
    assert(query.hasText());
    assert(query.text() == "Terra X");

    assert(query.hasBackend());
    assert(query.backendId() == "home-vdr");

    assert(query.hasChannelScope());
    assert(query.channelScope() == EpgSearchChannelScope::Interval);
    assert(query.channelMin() == "C-1-1051-10301");
    assert(query.channelMax() == "C-1-1051-10301");

    assert(query.hasFieldSelection());
    assert(query.useTitle());
    assert(!query.useSubtitle());
    assert(query.useDescription());

    std::cout
        << "test_epgsearch_request_mapper passed"
        << std::endl;

    return 0;
}
