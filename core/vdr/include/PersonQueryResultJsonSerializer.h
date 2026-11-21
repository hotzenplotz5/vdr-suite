#pragma once

#include "PersonQueryResult.h"

#include <string>

class PersonQueryResultJsonSerializer {
public:
    std::string serialize(
        const PersonQueryResult& result) const;
};
