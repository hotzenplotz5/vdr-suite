#pragma once

#include "DashboardController.h"

#include <string>

class SearchTimerResult;
class SearchTimerResultJsonSerializer;

class SearchTimerController
{
public:
    explicit SearchTimerController(
        SearchTimerResultJsonSerializer& jsonSerializer);

    ApiResponse getSearchTimers(
        const SearchTimerResult& result);

private:
    SearchTimerResultJsonSerializer& jsonSerializer_;
};