#pragma once

#include "DashboardController.h"
#include "RemoteActionRequestParser.h"
#include "RemoteActionService.h"

#include <functional>
#include <string>

class RemoteActionResultJsonSerializer
{
public:
    std::string serialize(const RemoteActionResult& result) const;
};

class RemoteActionController
{
public:
    using SuccessCallback = std::function<void(const RemoteActionRequest&)>;

    RemoteActionController(
        const RemoteActionRequestParser& parser,
        const RemoteActionService& service,
        const RemoteActionResultJsonSerializer& serializer,
        SuccessCallback successCallback = {});

    ApiResponse executeBody(const std::string& body) const;

    static int statusCodeFor(const RemoteActionResult& result);

private:
    const RemoteActionRequestParser& parser_;
    const RemoteActionService& service_;
    const RemoteActionResultJsonSerializer& serializer_;
    SuccessCallback successCallback_;
};
