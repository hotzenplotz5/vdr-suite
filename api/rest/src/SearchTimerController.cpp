#include "SearchTimerController.h"

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerService.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerPreviewResultJsonSerializer.h"
#include "SearchTimerPreviewService.h"

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
      dataSource_(nullptr)
{
}

SearchTimerController::SearchTimerController(
    SearchTimerService& searchTimerService,
    SearchTimerResultJsonSerializer& jsonSerializer,
    ISearchTimerDataSource& dataSource)
    : searchTimerService_(searchTimerService),
      jsonSerializer_(jsonSerializer),
      dataSource_(&dataSource)
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
