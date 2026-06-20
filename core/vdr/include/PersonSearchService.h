#pragma once

#include "Person.h"
#include "PersonQuery.h"
#include "PersonQueryResult.h"

class PersonSearchService {
public:
    PersonQueryResult search(
        const PersonCollection& persons,
        const PersonQuery& query,
        int limit,
        int offset) const;
};
