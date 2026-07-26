#pragma once

#include "IHttpClient.h"
#include "IRuntimeLogger.h"
#include "IRuntimeMeasurementSink.h"
#include "RuntimeLogEntry.h"
#include "RuntimeLogLevel.h"
#include "RuntimeMeasurement.h"

#include <functional>
#include <string>

class BasicHttpClient : public IHttpClient {
public:
    using CancellationCheck = std::function<bool()>;

    BasicHttpClient(
        std::string host,
        int port,
        IRuntimeLogger* logger = nullptr,
        IRuntimeMeasurementSink* measurementSink = nullptr,
        CancellationCheck cancellationCheck = {});

    HttpResponse execute(const HttpRequest& request) const override;

private:
    std::string host_;
    int port_;
    IRuntimeLogger* logger_;
    IRuntimeMeasurementSink* measurementSink_;
    CancellationCheck cancellationCheck_;

    bool cancellationRequested() const;
    void log(RuntimeLogLevel level, const std::string& message) const;
    void recordMeasurement(const RuntimeMeasurement& measurement) const;
};
