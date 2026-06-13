#include "RestQueryParameters.h"

#include <cassert>
#include <iostream>

int main()
{
    RestQueryParameters empty =
        RestQueryParameters::parse("");

    assert(!empty.has("channelId"));
    assert(empty.get("channelId", "default") == "default");
    assert(empty.getInt("from", -1) == -1);

    RestQueryParameters parameters =
        RestQueryParameters::parse(
            "channelId=channel-1&from=123&timespan=7200&limit=5");

    assert(parameters.has("channelId"));
    assert(parameters.get("channelId") == "channel-1");
    assert(parameters.getInt("from", -1) == 123);
    assert(parameters.getInt("timespan", 0) == 7200);
    assert(parameters.getInt("limit", 0) == 5);

    RestQueryParameters missingValue =
        RestQueryParameters::parse("onlyFlag&badInt=abc");

    assert(missingValue.has("onlyFlag"));
    assert(missingValue.get("onlyFlag", "x").empty());
    assert(missingValue.getInt("badInt", 99) == 99);

    RestQueryParameters repeated =
        RestQueryParameters::parse("limit=1&limit=2");

    assert(repeated.getInt("limit", 0) == 2);

    std::cout << "test_rest_query_parameters passed" << std::endl;

    return 0;
}
