#pragma once

#include "PersonContext.h"

#include <string>

class PersonContextJsonSerializer
{
public:
    std::string serialize(const PersonContextResult& result) const;
};
