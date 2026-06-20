#pragma once

#include "DashboardController.h"

#include <string>

class SearchTimerResult;
class SearchTimerResultJsonSerializer;
class SearchTimerService;

class SearchTimerController
{
public:
    SearchTimerController(
        SearchTimerService& searchTimerService,
        SearchTimerResultJsonSerializer& jsonSerializer);

    ApiResponse getSearchTimers(
        const SearchTimerResult& result);

    ApiResponse searchSearchTimers(
        const SearchTimerResult& source,
        const std::string& backend,
        const std::string& state,
        const std::string& text,
        int limit,
        int offset);

private:
    SearchTimerService& searchTimerService_;
    SearchTimerResultJsonSerializer& jsonSerializer_;
};