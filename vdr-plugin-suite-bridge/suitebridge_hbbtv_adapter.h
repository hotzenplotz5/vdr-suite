#ifndef VDR_SUITE_BRIDGE_HBBTV_ADAPTER_H
#define VDR_SUITE_BRIDGE_HBBTV_ADAPTER_H

#include "suitebridge_hbbtv_provider_contract.h"

#include <functional>
#include <string>

class ISuiteBridgeHbbtvProvider {
public:
  virtual ~ISuiteBridgeHbbtvProvider() = default;

  virtual bool Discover(
      const std::string &channelId,
      VdrWebHbbtvDiscoveryV1 &discovery,
      std::string &error) const = 0;

  virtual bool Runtime(
      VdrWebHbbtvRuntimeV1 &runtime,
      std::string &error) const = 0;

  virtual bool Presentation(
      VdrWebHbbtvPresentationV1 &presentation,
      std::string &error) const = 0;
};

class SuiteBridgeHbbtvAdapter final : public ISuiteBridgeHbbtvProvider {
public:
  using ServiceCaller = std::function<bool(const char *, void *)>;

  SuiteBridgeHbbtvAdapter();
  explicit SuiteBridgeHbbtvAdapter(ServiceCaller caller);

  bool Discover(
      const std::string &channelId,
      VdrWebHbbtvDiscoveryV1 &discovery,
      std::string &error) const override;

  bool Runtime(
      VdrWebHbbtvRuntimeV1 &runtime,
      std::string &error) const override;

  bool Presentation(
      VdrWebHbbtvPresentationV1 &presentation,
      std::string &error) const override;

private:
  ServiceCaller caller_;
};

#endif
