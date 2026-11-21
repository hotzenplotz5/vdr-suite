#ifndef VDR_SUITE_BRIDGE_TELETEXT_ADAPTER_H
#define VDR_SUITE_BRIDGE_TELETEXT_ADAPTER_H

#include "suitebridge_teletext_provider_contract.h"

#include <cstdint>
#include <functional>
#include <string>

class ISuiteBridgeTeletextProvider {
public:
  virtual ~ISuiteBridgeTeletextProvider() = default;

  virtual bool ReadCapabilities(
      OsdTeletextCapabilitiesV1 &capabilities,
      std::string &error) const = 0;

  virtual bool ReadPage(
      const std::string &channelId,
      std::uint16_t pageNumber,
      std::uint16_t subpageCode,
      OsdTeletextGetPageV1 &page,
      std::string &error) const = 0;
};

class SuiteBridgeTeletextAdapter final : public ISuiteBridgeTeletextProvider {
public:
  using ServiceCaller = std::function<bool(const char *, void *)>;

  SuiteBridgeTeletextAdapter();
  explicit SuiteBridgeTeletextAdapter(ServiceCaller caller);

  bool ReadCapabilities(
      OsdTeletextCapabilitiesV1 &capabilities,
      std::string &error) const override;

  bool ReadPage(
      const std::string &channelId,
      std::uint16_t pageNumber,
      std::uint16_t subpageCode,
      OsdTeletextGetPageV1 &page,
      std::string &error) const override;

private:
  bool ValidateCapabilities(
      const OsdTeletextCapabilitiesV1 &capabilities,
      std::string &error) const;

  ServiceCaller caller_;
};

#endif
