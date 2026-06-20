#pragma once

#include "Person.h"
#include "PersonQuery.h"

class PersonQueryMatcher {
public:
    bool matches(
        const Person& person,
        const PersonQuery& query) const;
};
