#pragma once

#include "DashboardController.h"
#include "Person.h"

#include <string>

class PersonQueryResultJsonSerializer;
class PersonResolutionJsonSerializer;
class PersonResolutionResult;
class PersonSearchService;

class PersonController
{
public:
    explicit PersonController(
        PersonResolutionJsonSerializer& jsonSerializer);

    PersonController(
        PersonResolutionJsonSerializer& resolutionJsonSerializer,
        PersonSearchService& searchService,
        PersonQueryResultJsonSerializer& queryResultJsonSerializer);

    ApiResponse getPersonResolution(
        const PersonResolutionResult& result);

    ApiResponse searchPersons(
        const PersonCollection& persons,
        const std::string& name,
        const std::string& normalizedName,
        const std::string& role,
        const std::string& source,
        const std::string& providerReference,
        int limit,
        int offset);

private:
    PersonResolutionJsonSerializer& resolutionJsonSerializer_;
    PersonSearchService* searchService_ = nullptr;
    PersonQueryResultJsonSerializer* queryResultJsonSerializer_ = nullptr;
};
