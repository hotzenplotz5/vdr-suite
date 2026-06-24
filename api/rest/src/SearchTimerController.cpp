#include "SearchTimerController.h"

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerService.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerCreateRequestParser.h"
#include "SearchTimerCreateResultJsonSerializer.h"
#include "SearchTimerCreateService.h"
#include "SearchTimerDeleteRequestParser.h"
#include "SearchTimerDeleteResultJsonSerializer.h"
#include "SearchTimerDeleteService.h"
#include "SearchTimerUpdateRequestParser.h"
#include "SearchTimerUpdateResultJsonSerializer.h"
#include "SearchTimerUpdateService.h"
#include "SearchTimerPreviewResultJsonSerializer.h"
#include "SearchTimerPreviewService.h"
#include "SearchTimerWorkflowValidationRequestParser.h"
#include "SearchTimerWorkflowExecutionPlanJsonSerializer.h"
#include "SearchTimerWorkflowExecutionResultJsonSerializer.h"
#include "SearchTimerWorkflowCommandDispatchService.h"
#include "SearchTimerWorkflowPlanningService.h"
#include "SearchTimerWorkflowValidationResultJsonSerializer.h"
#include "SearchTimerWorkflowValidationService.h"
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


bool parseExecutionConfirmationField(
    const std::string& body,
    const std::string& key)
{
    const std::string keyToken =
        "\"" + key + "\"";

    const std::size_t keyPosition =
        body.find(keyToken);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        body.find(':', keyPosition + keyToken.size());

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t valuePosition =
        body.find_first_not_of(" \t\n\r", colonPosition + 1);

    if (valuePosition == std::string::npos)
    {
        return false;
    }

    if (body.compare(valuePosition, 4, "true") == 0)
    {
        return true;
    }

    if (body.compare(valuePosition, 1, "1") == 0)
    {
        return true;
    }

    if (body.compare(valuePosition, 6, "\"true\"") == 0)
    {
        return true;
    }

    if (body.compare(valuePosition, 3, "\"1\"") == 0)
    {
        return true;
    }

    return false;
}

bool parseExecutionConfirmation(
    const std::string& body)
{
    return parseExecutionConfirmationField(
               body,
               "explicitOperatorConfirmation") ||
           parseExecutionConfirmationField(
               body,
               "operatorConfirmed") ||
           parseExecutionConfirmationField(
               body,
               "confirmed");
}

bool parseExecutorOptIn(
    const std::string& body)
{
    return parseExecutionConfirmationField(
               body,
               "executorOptIn") ||
           parseExecutionConfirmationField(
               body,
               "executorOptInEnabled") ||
           parseExecutionConfirmationField(
               body,
               "executorOptInProvided") ||
           parseExecutionConfirmationField(
               body,
               "enableExecutor") ||
           parseExecutionConfirmationField(
               body,
               "allowExecutor");
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
      updateRequestParser_(nullptr),
      deleteService_(nullptr),
      deleteJsonSerializer_(nullptr),
      deleteRequestParser_(nullptr)
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
      updateRequestParser_(nullptr),
      deleteService_(nullptr),
      deleteJsonSerializer_(nullptr),
      deleteRequestParser_(nullptr)
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
    SearchTimerUpdateRequestParser* updateRequestParser,
    SearchTimerDeleteService* deleteService,
    SearchTimerDeleteResultJsonSerializer* deleteJsonSerializer,
    SearchTimerDeleteRequestParser* deleteRequestParser)
    : searchTimerService_(searchTimerService),
      jsonSerializer_(jsonSerializer),
      dataSource_(&dataSource),
      createService_(&createService),
      createJsonSerializer_(&createJsonSerializer),
      createRequestParser_(&createRequestParser),
      updateService_(updateService),
      updateJsonSerializer_(updateJsonSerializer),
      updateRequestParser_(updateRequestParser),
      deleteService_(deleteService),
      deleteJsonSerializer_(deleteJsonSerializer),
      deleteRequestParser_(deleteRequestParser)
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

ApiResponse SearchTimerController::deleteSearchTimer(
    const std::string& body,
    ISearchTimerCommandExecutor& executor)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";

    if (deleteService_ == nullptr ||
        deleteJsonSerializer_ == nullptr ||
        deleteRequestParser_ == nullptr)
    {
        response.statusCode = 500;
        response.body = "{\"error\":\"searchtimer delete service unavailable\"}";
        return response;
    }

    response.body =
        deleteJsonSerializer_->serialize(
            deleteService_->remove(
                deleteRequestParser_->parse(body),
                executor));

    return response;
}


ApiResponse SearchTimerController::validateSearchTimerWorkflow(
    const std::string& body)
{
    SearchTimerWorkflowValidationService validationService;
    SearchTimerWorkflowValidationRequestParser requestParser;
    SearchTimerWorkflowValidationResultJsonSerializer validationJsonSerializer;

    const SearchTimerWorkflowValidationResult result =
        validationService.validate(
            requestParser.parse(body));

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        validationJsonSerializer.serialize(result);

    return response;
}

ApiResponse SearchTimerController::planSearchTimerWorkflow(
    const std::string& body)
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowValidationRequestParser requestParser;
    SearchTimerWorkflowExecutionPlanJsonSerializer planJsonSerializer;

    const SearchTimerWorkflowExecutionPlan plan =
        planningService.plan(
            requestParser.parse(body));

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        planJsonSerializer.serialize(plan);

    return response;
}


ApiResponse SearchTimerController::executeSearchTimerWorkflow(
    const std::string& body)
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowValidationRequestParser requestParser;
    SearchTimerWorkflowCommandDispatchService dispatchService;
    SearchTimerWorkflowExecutionResultJsonSerializer resultJsonSerializer;

    const SearchTimerWorkflowExecutionPlan plan =
        planningService.plan(
            requestParser.parse(body));

    const bool explicitOperatorConfirmation =
        parseExecutionConfirmation(body);

    const bool executorOptIn =
        parseExecutorOptIn(body);

    const SearchTimerWorkflowCommandDispatchOptions dispatchOptions =
        executorOptIn
            ? SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(
                  explicitOperatorConfirmation)
            : SearchTimerWorkflowCommandDispatchOptions::confirmed(
                  explicitOperatorConfirmation);

    const SearchTimerWorkflowExecutionResult result =
        dispatchService.dispatchPlan(
            plan,
            dispatchOptions);

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        resultJsonSerializer.serialize(result);

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
