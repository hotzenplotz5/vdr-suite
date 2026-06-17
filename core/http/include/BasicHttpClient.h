#pragma once

#include "IHttpClient.h"
#include "IRuntimeLogger.h"
#include "IRuntimeMeasurementSink.h"
#include "RuntimeLogEntry.h"
#include "RuntimeLogLevel.h"
#include "RuntimeMeasurement.h"

#include <string>

class BasicHttpClient : public IHttpClient {
public:
    BasicHttpClient(
        std::string host,
        int port,
        IRuntimeLogger* logger = nullptr,
        IRuntimeMeasurementSink* measurementSink = nullptr);

    HttpResponse execute(const HttpRequest& request) const override;

private:
    std::string host_;
    int port_;
    IRuntimeLogger* logger_;
    IRuntimeMeasurementSink* measurementSink_;

    void log(RuntimeLogLevel level, const std::string& message) const;
    void recordMeasurement(const RuntimeMeasurement& measurement) const;
};
