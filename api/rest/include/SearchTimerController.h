#pragma once

#include "DashboardController.h"

#include <string>

class ISearchTimerDataSource;
class SearchTimerResult;
class SearchTimerResultJsonSerializer;
class SearchTimerService;

class SearchTimerController
{
public:
    SearchTimerController(
        SearchTimerService& searchTimerService,
        SearchTimerResultJsonSerializer& jsonSerializer);

    SearchTimerController(
        SearchTimerService& searchTimerService,
        SearchTimerResultJsonSerializer& jsonSerializer,
        ISearchTimerDataSource& dataSource);

    ApiResponse getSearchTimers(
        const SearchTimerResult& result);

    ApiResponse searchSearchTimers(
        const SearchTimerResult& source,
        const std::string& backend,
        const std::string& state,
        const std::string& text,
        int limit,
        int offset);

    ApiResponse searchSearchTimers(
        const std::string& backend,
        const std::string& state,
        const std::string& text,
        int limit,
        int offset);

private:
    SearchTimerService& searchTimerService_;
    SearchTimerResultJsonSerializer& jsonSerializer_;
    ISearchTimerDataSource* dataSource_;
};