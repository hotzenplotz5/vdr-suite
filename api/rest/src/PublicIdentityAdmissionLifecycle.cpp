#include "PublicApiRuntime.h"

#include <mutex>
#include <utility>

// ADR-0068: Account / Device callback and admission lifecycle.
// Original callback slots, mutexes, invoke/lookup semantics and owner are
// unchanged. The daemon still registers and resets the single runtime.
void PublicApiRuntime::registerDevicePairingCreate(
    DevicePairingCreate create)
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    devicePairingCreate_ = std::move(create);
}

void PublicApiRuntime::resetDevicePairingCreate()
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    devicePairingCreate_ = {};
}

bool PublicApiRuntime::devicePairingCreateConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    return static_cast<bool>(devicePairingCreate_);
}

void PublicApiRuntime::registerDevicePairingLookup(
    DevicePairingLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    devicePairingLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    devicePairingLookup_ = {};
}

bool PublicApiRuntime::devicePairingLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    return static_cast<bool>(devicePairingLookup_);
}

void PublicApiRuntime::registerDeviceCredentialIssue(
    DeviceCredentialIssue issue)
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    deviceCredentialIssue_ = std::move(issue);
}

void PublicApiRuntime::resetDeviceCredentialIssue()
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    deviceCredentialIssue_ = {};
}

bool PublicApiRuntime::deviceCredentialIssueConfigured() const
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    return static_cast<bool>(deviceCredentialIssue_);
}

void PublicApiRuntime::registerDevicePairingAdministrationCollectionLookup(
    DevicePairingAdministrationCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    devicePairingAdministrationCollectionLookup_ =
        std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingAdministrationCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    devicePairingAdministrationCollectionLookup_ = {};
}

bool PublicApiRuntime::
devicePairingAdministrationCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    return static_cast<bool>(
        devicePairingAdministrationCollectionLookup_);
}

void PublicApiRuntime::registerDevicePairingAdministrationLookup(
    DevicePairingAdministrationLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    devicePairingAdministrationLookup_ =
        std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingAdministrationLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    devicePairingAdministrationLookup_ = {};
}

bool PublicApiRuntime::
devicePairingAdministrationLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    return static_cast<bool>(
        devicePairingAdministrationLookup_);
}

void PublicApiRuntime::registerDevicePairingDecision(
    DevicePairingDecision decision)
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    devicePairingDecision_ =
        std::move(decision);
}

void PublicApiRuntime::resetDevicePairingDecision()
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    devicePairingDecision_ = {};
}

bool PublicApiRuntime::devicePairingDecisionConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    return static_cast<bool>(devicePairingDecision_);
}

void PublicApiRuntime::registerAccountCollectionLookup(
    AccountCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    accountCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    accountCollectionLookup_ = {};
}

bool PublicApiRuntime::accountCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    return static_cast<bool>(accountCollectionLookup_);
}

PublicAccountCollectionResult
PublicApiRuntime::lookupAccountCollection(
    const PublicAccountCollectionRequest& request) const
{
    AccountCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            accountCollectionLookupMutex_);
        lookup = accountCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerAccountLookup(
    AccountLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    accountLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountLookup()
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    accountLookup_ = {};
}

bool PublicApiRuntime::accountLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    return static_cast<bool>(accountLookup_);
}

PublicAccountLookupResult
PublicApiRuntime::lookupAccount(
    const std::string& accountId) const
{
    AccountLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            accountLookupMutex_);
        lookup = accountLookup_;
    }
    if (!lookup) return {};
    return lookup(accountId);
}

void PublicApiRuntime::registerAccountMutation(
    AccountMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    accountMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountMutation()
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    accountMutation_ = {};
}

bool PublicApiRuntime::accountMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    return static_cast<bool>(accountMutation_);
}

void PublicApiRuntime::registerAccountCreate(
    AccountCreate create)
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    accountCreate_ = std::move(create);
}

void PublicApiRuntime::resetAccountCreate()
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    accountCreate_ = {};
}

bool PublicApiRuntime::accountCreateConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    return static_cast<bool>(accountCreate_);
}

void PublicApiRuntime::registerDeviceLifecycleLookup(
    DeviceLifecycleLookup lookup)
{
    std::lock_guard<std::mutex> lock(deviceLifecycleLookupMutex_);
    deviceLifecycleLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDeviceLifecycleLookup()
{
    std::lock_guard<std::mutex> lock(deviceLifecycleLookupMutex_);
    deviceLifecycleLookup_ = {};
}

void PublicApiRuntime::registerDeviceLifecycleMutation(
    DeviceLifecycleMutation mutation)
{
    std::lock_guard<std::mutex> lock(deviceLifecycleMutationMutex_);
    deviceLifecycleMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetDeviceLifecycleMutation()
{
    std::lock_guard<std::mutex> lock(deviceLifecycleMutationMutex_);
    deviceLifecycleMutation_ = {};
}

bool PublicApiRuntime::deviceLifecycleAdministrationConfigured() const
{
    std::lock_guard<std::mutex> a(deviceLifecycleLookupMutex_);
    std::lock_guard<std::mutex> b(deviceLifecycleMutationMutex_);
    return static_cast<bool>(deviceLifecycleLookup_) &&
           static_cast<bool>(deviceLifecycleMutation_);
}

void PublicApiRuntime::registerDeviceCredentialRotation(
    DeviceCredentialRotation rotate)
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    deviceCredentialRotation_ = std::move(rotate);
}

void PublicApiRuntime::resetDeviceCredentialRotation()
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    deviceCredentialRotation_ = {};
}

bool PublicApiRuntime::deviceCredentialRotationConfigured() const
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    return static_cast<bool>(deviceCredentialRotation_);
}

void PublicApiRuntime::registerDeviceCollectionLookup(
    DeviceCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(deviceCollectionLookupMutex_);
    deviceCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDeviceCollectionLookup()
{
    std::lock_guard<std::mutex> lock(deviceCollectionLookupMutex_);
    deviceCollectionLookup_ = {};
}

void PublicApiRuntime::registerDeviceGrantLookup(
    DeviceGrantLookup lookup)
{
    std::lock_guard<std::mutex> lock(deviceGrantLookupMutex_);
    deviceGrantLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDeviceGrantLookup()
{
    std::lock_guard<std::mutex> lock(deviceGrantLookupMutex_);
    deviceGrantLookup_ = {};
}

void PublicApiRuntime::registerDeviceGrantMutation(
    DeviceGrantMutation mutation)
{
    std::lock_guard<std::mutex> lock(deviceGrantMutationMutex_);
    deviceGrantMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetDeviceGrantMutation()
{
    std::lock_guard<std::mutex> lock(deviceGrantMutationMutex_);
    deviceGrantMutation_ = {};
}

bool PublicApiRuntime::deviceGrantAdministrationConfigured() const
{
    std::lock_guard<std::mutex> lookupLock(deviceGrantLookupMutex_);
    std::lock_guard<std::mutex> mutationLock(deviceGrantMutationMutex_);
    return static_cast<bool>(deviceGrantLookup_) &&
           static_cast<bool>(deviceGrantMutation_);
}

void PublicApiRuntime::registerAccountGrantLookup(
    AccountGrantLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    accountGrantLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountGrantLookup()
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    accountGrantLookup_ = {};
}

bool PublicApiRuntime::accountGrantLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    return static_cast<bool>(accountGrantLookup_);
}

void PublicApiRuntime::registerAccountGrantMutation(
    AccountGrantMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    accountGrantMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountGrantMutation()
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    accountGrantMutation_ = {};
}

bool PublicApiRuntime::accountGrantMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    return static_cast<bool>(accountGrantMutation_);
}

void PublicApiRuntime::registerAccountCredentialLookup(
    AccountCredentialLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    accountCredentialLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCredentialLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    accountCredentialLookup_ = {};
}

bool PublicApiRuntime::accountCredentialLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    return static_cast<bool>(accountCredentialLookup_);
}

void PublicApiRuntime::registerAccountCredentialItemLookup(
    AccountCredentialItemLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    accountCredentialItemLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCredentialItemLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    accountCredentialItemLookup_ = {};
}

bool PublicApiRuntime::accountCredentialItemLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    return static_cast<bool>(accountCredentialItemLookup_);
}

void PublicApiRuntime::registerAccountCredentialMutation(
    AccountCredentialMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    accountCredentialMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountCredentialMutation()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    accountCredentialMutation_ = {};
}

bool PublicApiRuntime::accountCredentialMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    return static_cast<bool>(accountCredentialMutation_);
}

void PublicApiRuntime::registerAccountSessionLookup(
    AccountSessionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    accountSessionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountSessionLookup()
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    accountSessionLookup_ = {};
}

bool PublicApiRuntime::accountSessionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    return static_cast<bool>(accountSessionLookup_);
}

void PublicApiRuntime::registerAccountSessionItemLookup(
    AccountSessionItemLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    accountSessionItemLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountSessionItemLookup()
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    accountSessionItemLookup_ = {};
}

bool PublicApiRuntime::accountSessionItemLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    return static_cast<bool>(accountSessionItemLookup_);
}

void PublicApiRuntime::registerAccountSessionMutation(
    AccountSessionMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    accountSessionMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountSessionMutation()
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    accountSessionMutation_ = {};
}

bool PublicApiRuntime::accountSessionMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    return static_cast<bool>(accountSessionMutation_);
}

void PublicApiRuntime::registerNativeTimerModifyAdmission(
    NativeTimerModifyAdmission admission)
{
    std::lock_guard<std::mutex> lock(nativeTimerModifyMutex_);
    nativeTimerModifyAdmission_ = std::move(admission);
}

void PublicApiRuntime::resetNativeTimerModifyAdmission()
{
    std::lock_guard<std::mutex> lock(nativeTimerModifyMutex_);
    nativeTimerModifyAdmission_ = {};
}

void PublicApiRuntime::registerNativeTimerDeleteAdmission(
    NativeTimerDeleteAdmission admission)
{
    std::lock_guard<std::mutex> lock(nativeTimerDeleteMutex_);
    nativeTimerDeleteAdmission_ = std::move(admission);
}

void PublicApiRuntime::resetNativeTimerDeleteAdmission()
{
    std::lock_guard<std::mutex> lock(nativeTimerDeleteMutex_);
    nativeTimerDeleteAdmission_ = {};
}
