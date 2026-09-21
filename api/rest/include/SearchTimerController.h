#pragma once

#include "DashboardController.h"

#include <string>
#include <vector>

#include "SearchTimer.h"
#include "VdrEvent.h"

class ISearchTimerCommandExecutor;
class ISearchTimerDataSource;
class SearchTimerCreateRequestParser;
class SearchTimerCreateResultJsonSerializer;
class SearchTimerCreateService;
class SearchTimerDeleteRequestParser;
class SearchTimerDeleteResultJsonSerializer;
class SearchTimerDeleteService;
class SearchTimerResult;
class SearchTimerResultJsonSerializer;
class SearchTimerService;
class SearchTimerUpdateRequestParser;
class SearchTimerUpdateResultJsonSerializer;
class SearchTimerUpdateService;

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

    SearchTimerController(
        SearchTimerService& searchTimerService,
        SearchTimerResultJsonSerializer& jsonSerializer,
        ISearchTimerDataSource& dataSource,
        SearchTimerCreateService& createService,
        SearchTimerCreateResultJsonSerializer& createJsonSerializer,
        SearchTimerCreateRequestParser& createRequestParser,
        SearchTimerUpdateService* updateService = nullptr,
        SearchTimerUpdateResultJsonSerializer* updateJsonSerializer = nullptr,
        SearchTimerUpdateRequestParser* updateRequestParser = nullptr,
        SearchTimerDeleteService* deleteService = nullptr,
        SearchTimerDeleteResultJsonSerializer* deleteJsonSerializer = nullptr,
        SearchTimerDeleteRequestParser* deleteRequestParser = nullptr);

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

    ApiResponse previewSearchTimer(
        const SearchTimer& searchTimer,
        const std::vector<VdrEvent>& events,
        int limit,
        int offset);

    ApiResponse createSearchTimer(
        const std::string& body,
        ISearchTimerCommandExecutor& executor);

    ApiResponse updateSearchTimer(
        const std::string& body,
        ISearchTimerCommandExecutor& executor);

    ApiResponse deleteSearchTimer(
        const std::string& body,
        ISearchTimerCommandExecutor& executor);

    ApiResponse validateSearchTimerWorkflow(
        const std::string& body);

    ApiResponse planSearchTimerWorkflow(
        const std::string& body);

    ApiResponse executeSearchTimerWorkflow(
        const std::string& body);

    ApiResponse realTestSearchTimerWorkflow(
        const std::string& body);

private:
    SearchTimerService& searchTimerService_;
    SearchTimerResultJsonSerializer& jsonSerializer_;
    ISearchTimerDataSource* dataSource_;
    SearchTimerCreateService* createService_;
    SearchTimerCreateResultJsonSerializer* createJsonSerializer_;
    SearchTimerCreateRequestParser* createRequestParser_;
    SearchTimerUpdateService* updateService_;
    SearchTimerUpdateResultJsonSerializer* updateJsonSerializer_;
    SearchTimerUpdateRequestParser* updateRequestParser_;
    SearchTimerDeleteService* deleteService_;
    SearchTimerDeleteResultJsonSerializer* deleteJsonSerializer_;
    SearchTimerDeleteRequestParser* deleteRequestParser_;
};