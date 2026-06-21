#include "SearchTimerController.h"

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerService.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerCreateRequestParser.h"
#include "SearchTimerCreateResultJsonSerializer.h"
#include "SearchTimerCreateService.h"
#include "SearchTimerUpdateRequestParser.h"
#include "SearchTimerUpdateResultJsonSerializer.h"
#include "SearchTimerUpdateService.h"
#include "SearchTimerPreviewResultJsonSerializer.h"
#include "SearchTimerPreviewService.h"
#include "ISearchTimerCommandExecutor.h"

namespace {

SearchTimerQuery buildSearchTimerQuery(
    const std::string& backend,
    const std::string& state,
    const std::string& text,
    int limit,
    int offset)
{
    SearchTimerQuery query = SearchTimerQuery::limited(limit, offset);

    if (!backend.empty()) {
        query = query.withBackend(backend);
    }

    if (!text.empty()) {
        query = query.withText(text);
    }

    if (state == "active") {
        query = query.withState(SearchTimerState::Active);
    } else if (state == "inactive") {
        query = query.withState(SearchTimerState::Inactive);
    }

    return query;
}

} // namespace

SearchTimerController::SearchTimerController(
    SearchTimerService& searchTimerService,
    SearchTimerResultJsonSerializer& jsonSerializer)
    : searchTimerService_(searchTimerService),
      jsonSerializer_(jsonSerializer),
      dataSource_(nullptr),
      createService_(nullptr),
      createJsonSerializer_(nullptr),
      createRequestParser_(nullptr),
      updateService_(nullptr),
      updateJsonSerializer_(nullptr),
      updateRequestParser_(nullptr)
{
}

SearchTimerController::SearchTimerController(
    SearchTimerService& searchTimerService,
    SearchTimerResultJsonSerializer& jsonSerializer,
    ISearchTimerDataSource& dataSource)
    : searchTimerService_(searchTimerService),
      jsonSerializer_(jsonSerializer),
      dataSource_(&dataSource),
      createService_(nullptr),
      createJsonSerializer_(nullptr),
      createRequestParser_(nullptr),
      updateService_(nullptr),
      updateJsonSerializer_(nullptr),
      updateRequestParser_(nullptr)
{
}

SearchTimerController::SearchTimerController(
    SearchTimerService& searchTimerService,
    SearchTimerResultJsonSerializer& jsonSerializer,
    ISearchTimerDataSource& dataSource,
    SearchTimerCreateService& createService,
    SearchTimerCreateResultJsonSerializer& createJsonSerializer,
    SearchTimerCreateRequestParser& createRequestParser,
    SearchTimerUpdateService* updateService,
    SearchTimerUpdateResultJsonSerializer* updateJsonSerializer,
    SearchTimerUpdateRequestParser* updateRequestParser)
    : searchTimerService_(searchTimerService),
      jsonSerializer_(jsonSerializer),
      dataSource_(&dataSource),
      createService_(&createService),
      createJsonSerializer_(&createJsonSerializer),
      createRequestParser_(&createRequestParser),
      updateService_(updateService),
      updateJsonSerializer_(updateJsonSerializer),
      updateRequestParser_(updateRequestParser)
{
}

ApiResponse SearchTimerController::getSearchTimers(
    const SearchTimerResult& result)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(result);

    return response;
}
ApiResponse SearchTimerController::searchSearchTimers(
    const SearchTimerResult& source,
    const std::string& backend,
    const std::string& state,
    const std::string& text,
    int limit,
    int offset)
{
    SearchTimerResult result = searchTimerService_.list(
        source.items(),
        buildSearchTimerQuery(
            backend,
            state,
            text,
            limit,
            offset));

    return getSearchTimers(result);
}

ApiResponse SearchTimerController::createSearchTimer(
    const std::string& body,
    ISearchTimerCommandExecutor& executor)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";

    if (createService_ == nullptr ||
        createJsonSerializer_ == nullptr ||
        createRequestParser_ == nullptr)
    {
        response.statusCode = 500;
        response.body = "{\"error\":\"searchtimer create service unavailable\"}";
        return response;
    }

    response.body =
        createJsonSerializer_->serialize(
            createService_->create(
                createRequestParser_->parse(body),
                executor));

    return response;
}

ApiResponse SearchTimerController::updateSearchTimer(
    const std::string& body,
    ISearchTimerCommandExecutor& executor)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";

    if (updateService_ == nullptr ||
        updateJsonSerializer_ == nullptr ||
        updateRequestParser_ == nullptr)
    {
        response.statusCode = 500;
        response.body = "{\"error\":\"searchtimer update service unavailable\"}";
        return response;
    }

    response.body =
        updateJsonSerializer_->serialize(
            updateService_->update(
                updateRequestParser_->parse(body),
                executor));

    return response;
}

ApiResponse SearchTimerController::previewSearchTimer(
    const SearchTimer& searchTimer,
    const std::vector<VdrEvent>& events,
    int limit,
    int offset)
{
    SearchTimerPreviewService previewService;
    SearchTimerPreviewResultJsonSerializer serializer;

    const SearchTimerPreviewResult result =
        previewService.preview(
            searchTimer,
            events,
            limit,
            offset);

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializer.serialize(result);

    return response;
}
ApiResponse SearchTimerController::searchSearchTimers(
    const std::string& backend,
    const std::string& state,
    const std::string& text,
    int limit,
    int offset)
{
    if (dataSource_ == nullptr) {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"error\":\"searchtimer data source unavailable\"}";
        return response;
    }

    SearchTimerResult result = dataSource_->list(
        buildSearchTimerQuery(
            backend,
            state,
            text,
            limit,
            offset));

    return getSearchTimers(result);
}
