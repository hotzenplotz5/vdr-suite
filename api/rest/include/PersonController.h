#pragma once

#include "DashboardController.h"

class PersonResolutionJsonSerializer;
class PersonResolutionResult;

class PersonController
{
public:
    explicit PersonController(
        PersonResolutionJsonSerializer& jsonSerializer);

    ApiResponse getPersonResolution(
        const PersonResolutionResult& result);

private:
    PersonResolutionJsonSerializer& jsonSerializer_;
};
