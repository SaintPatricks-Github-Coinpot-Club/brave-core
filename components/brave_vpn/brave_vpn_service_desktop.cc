/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/brave_vpn/url_constants.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace {

constexpr char kBraveVPNEntryName[] = "BraveVPN";

constexpr char kRegionContinentKey[] = "continent";
constexpr char kRegionNameKey[] = "name";
constexpr char kRegionNamePrettyKey[] = "name-pretty";

bool IsValidRegion(const brave_vpn::mojom::Region& region) {
  if (region.continent.empty() || region.name.empty() ||
      region.name_pretty.empty())
    return false;

  return true;
}

std::string GetBraveVPNPaymentsEnv() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(brave_vpn::switches::kBraveVPNPaymentsEnv)) {
#if defined(OFFICIAL_BUILD)
    return "";
#else
    return "development";
#endif
  }

  return cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNPaymentsEnv);
}

brave_vpn::BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(brave_vpn::switches::kBraveVPNSimulation))
    return brave_vpn::BraveVPNOSConnectionAPI::GetInstanceForTest();
  return brave_vpn::BraveVPNOSConnectionAPI::GetInstance();
}

}  // namespace

BraveVpnServiceDesktop::BraveVpnServiceDesktop(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : BraveVpnService(url_loader_factory), prefs_(prefs) {
  DCHECK(brave_vpn::IsBraveVPNEnabled());

  observed_.Observe(GetBraveVPNConnectionAPI());

  GetBraveVPNConnectionAPI()->set_target_vpn_entry_name(kBraveVPNEntryName);

  // Load cached data.
  LoadCachedRegionData();
  LoadPurchasedState();
  LoadSelectedRegion();

  CheckConnectionStateIfNeeded();

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kSkusVPNCredential,
      base::BindRepeating(&BraveVpnServiceDesktop::OnSkusVPNCredentialUpdated,
                          base::Unretained(this)));
}

BraveVpnServiceDesktop::~BraveVpnServiceDesktop() = default;

void BraveVpnServiceDesktop::ScheduleFetchRegionDataIfNeeded() {
  if (!is_purchased_user())
    return;

  if (region_data_update_timer_.IsRunning())
    return;

  // Try to update region list every 5h.
  FetchRegionData();
  constexpr int kRegionDataUpdateIntervalInHours = 5;
  region_data_update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromHours(kRegionDataUpdateIntervalInHours),
      this, &BraveVpnServiceDesktop::FetchRegionData);
}

void BraveVpnServiceDesktop::Shutdown() {
  BraveVpnService::Shutdown();

  observed_.Reset();
  receivers_.Clear();
  observers_.Clear();
  pref_change_registrar_.RemoveAll();
}

void BraveVpnServiceDesktop::OnCreated(const std::string& name) {
  if (cancel_connecting_) {
    cancel_connecting_ = false;
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    return;
  }

  for (const auto& obs : observers_)
    obs->OnConnectionCreated();

  asked_connecting_to_os_ = true;
  GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::OnCreateFailed(const std::string& name) {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnServiceDesktop::OnRemoved(const std::string& name) {
  for (const auto& obs : observers_)
    obs->OnConnectionRemoved();
}

void BraveVpnServiceDesktop::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  VLOG(2) << __func__ << " : current: " << static_cast<int>(connection_state_)
          << " : new: " << static_cast<int>(state);
  if (connection_state_ == state)
    return;

  // On Windows, we get disconnected status update twice.
  // When user connects to different region while connected,
  // we disconnect current connection and connect to newly selected
  // region. To do that we monitor |DISCONNECTED| state and start
  // connect when we get that state. But, Windows sends disconnected state
  // noti again. So, ignore second one.
  if (connection_state_ == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED) {
    // Ignore this state changing.
    return;
  }

  connection_state_ = state;
  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(connection_state_);
}

void BraveVpnServiceDesktop::OnConnected(const std::string& name) {
  VLOG(2) << __func__;

  asked_connecting_to_os_ = false;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);

  if (cancel_connecting_) {
    cancel_connecting_ = false;
    Disconnect();
    return;
  }
}

void BraveVpnServiceDesktop::OnIsConnecting(const std::string& name) {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
}

void BraveVpnServiceDesktop::OnConnectFailed(const std::string& name) {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnServiceDesktop::OnDisconnected(const std::string& name) {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);

  if (needs_connect_) {
    needs_connect_ = false;
    Connect();
  }
}

void BraveVpnServiceDesktop::OnIsDisconnecting(const std::string& name) {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVpnServiceDesktop::CreateVPNConnection() {
  if (cancel_connecting_) {
    cancel_connecting_ = false;
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    return;
  }

  VLOG(2) << __func__;
  GetBraveVPNConnectionAPI()->CreateVPNConnection(GetConnectionInfo());
}

void BraveVpnServiceDesktop::RemoveVPNConnnection() {
  VLOG(2) << __func__;
  GetBraveVPNConnectionAPI()->RemoveVPNConnection(
      GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Connect() {
  if (connection_state_ == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__
            << " : prevent connecting while disconnecting is in-progress";
    return;
  }

  if (connection_state_ == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << " : connecting in progress";
    return;
  }

  // Reset connect related states.
  cancel_connecting_ = false;
  asked_connecting_to_os_ = false;

  // User can ask connect again when user want to change region.
  if (connection_state_ == ConnectionState::CONNECTED) {
    // Disconnect first and then create again to setup for new region.
    needs_connect_ = true;
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  // If user doesn't select region explicitely, use default device region.
  std::string target_region_name = device_region_.name;
  if (IsValidRegion(selected_region_)) {
    target_region_name = selected_region_.name;
    VLOG(2) << __func__ << " : start connecting with valid selected_region: " << target_region_name;
  }
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
  FetchHostnamesForRegion(target_region_name);
}

void BraveVpnServiceDesktop::Disconnect() {
  if (connection_state_ == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__ << " : disconnecting in progress";
    return;
  }

  if (connection_state_ == ConnectionState::CONNECTING &&
      !asked_connecting_to_os_) {
    VLOG(2) << __func__ << " : cancel connecting";
    cancel_connecting_ = true;
    asked_connecting_to_os_ = false;
    // Start disconnecting.
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
    return;
  }

  VLOG(2) << __func__ << " : start disconnecting!";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
  GetBraveVPNConnectionAPI()->Disconnect(kBraveVPNEntryName);
}

void BraveVpnServiceDesktop::ToggleConnection() {
  const bool can_disconnect =
      (connection_state_ == ConnectionState::CONNECTED ||
       connection_state_ == ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

void BraveVpnServiceDesktop::AddObserver(
    mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

brave_vpn::BraveVPNConnectionInfo BraveVpnServiceDesktop::GetConnectionInfo() {
  return connection_info_;
}

void BraveVpnServiceDesktop::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveVpnServiceDesktop::GetConnectionState(
    GetConnectionStateCallback callback) {
  VLOG(2) << __func__ << " : " << static_cast<int>(connection_state_);
  std::move(callback).Run(connection_state_);
}

void BraveVpnServiceDesktop::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  VLOG(2) << __func__ << " : " << static_cast<int>(purchased_state_);
  std::move(callback).Run(purchased_state_);
}

void BraveVpnServiceDesktop::FetchRegionData() {
  VLOG(2) << __func__ << " : Start fetching region data";
  // Unretained is safe here becasue this class owns request helper.
  GetAllServerRegions(base::BindOnce(&BraveVpnServiceDesktop::OnFetchRegionList,
                                     base::Unretained(this)));
}

void BraveVpnServiceDesktop::LoadCachedRegionData() {
  auto* preference =
      prefs_->FindPreference(brave_vpn::prefs::kBraveVPNRegionList);
  if (preference && !preference->IsDefaultValue()) {
    ParseAndCacheRegionList(preference->GetValue()->Clone());
    VLOG(2) << __func__ << " : "
            << "Loaded cached region list";
  }

  preference = prefs_->FindPreference(brave_vpn::prefs::kBraveVPNDeviceRegion);
  if (preference && !preference->IsDefaultValue()) {
    auto* region_value = preference->GetValue();
    const std::string* continent =
        region_value->FindStringKey(kRegionContinentKey);
    const std::string* name = region_value->FindStringKey(kRegionNameKey);
    const std::string* name_pretty =
        region_value->FindStringKey(kRegionNamePrettyKey);
    if (continent && name && name_pretty) {
      device_region_.continent = *continent;
      device_region_.name = *name;
      device_region_.name_pretty = *name_pretty;
      VLOG(2) << __func__ << " : "
              << "Loaded cached device region";
    }
  }
}

void BraveVpnServiceDesktop::LoadPurchasedState() {
  const std::string credential =
      prefs_->GetString(brave_rewards::prefs::kSkusVPNCredential);
  skus_credential_ = credential;
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(brave_vpn::switches::kBraveVPNTestMonthlyPass)) {
    skus_credential_ =
        cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNTestMonthlyPass);
  }

  if (!skus_credential_.empty()) {
    SetPurchasedState(PurchasedState::PURCHASED);
    VLOG(2) << __func__ << " : "
            << "Loaded cached skus credentials";
  }
}

void BraveVpnServiceDesktop::LoadSelectedRegion() {
  auto* preference =
      prefs_->FindPreference(brave_vpn::prefs::kBraveVPNSelectedRegion);
  if (preference && !preference->IsDefaultValue()) {
    auto* region_value = preference->GetValue();
    const std::string* continent =
        region_value->FindStringKey(kRegionContinentKey);
    const std::string* name = region_value->FindStringKey(kRegionNameKey);
    const std::string* name_pretty =
        region_value->FindStringKey(kRegionNamePrettyKey);
    if (continent && name && name_pretty) {
      selected_region_.continent = *continent;
      selected_region_.name = *name;
      selected_region_.name_pretty = *name_pretty;
      VLOG(2) << __func__ << " : "
              << "Loaded selected region";
    }
  }
}

void BraveVpnServiceDesktop::CheckConnectionStateIfNeeded() {
  // If already in purchased state, there is a high probability that
  // the user has previously connected. So, try connection checking.
  if (is_purchased_user())
    GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);
}

void BraveVpnServiceDesktop::OnFetchRegionList(const std::string& region_list,
                                               bool success) {
  if (!success) {
    // TODO(simonhong): Re-try?
    VLOG(2) << "Failed to get region list";
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list()) {
    prefs_->Set(brave_vpn::prefs::kBraveVPNRegionList, *value);

    if (ParseAndCacheRegionList(std::move(*value))) {
      // Fetch timezones list to determine default region of this device.
      GetTimezonesForRegions(base::BindOnce(
          &BraveVpnServiceDesktop::OnFetchTimezones, base::Unretained(this)));
      return;
    }
  }

  // TODO(simonhong): Re-try?
}

bool BraveVpnServiceDesktop::ParseAndCacheRegionList(base::Value region_value) {
  DCHECK(region_value.is_list());
  if (!region_value.is_list())
    return false;

  regions_.clear();
  for (const auto& value : region_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    brave_vpn::mojom::Region region;
    if (auto* continent = value.FindStringKey(kRegionContinentKey))
      region.continent = *continent;
    if (auto* name = value.FindStringKey(kRegionNameKey))
      region.name = *name;
    if (auto* name_pretty = value.FindStringKey(kRegionNamePrettyKey))
      region.name_pretty = *name_pretty;

    regions_.push_back(region);
  }

  // Sort region list alphabetically
  std::sort(regions_.begin(), regions_.end(),
            [](brave_vpn::mojom::Region& a, brave_vpn::mojom::Region& b) {
              return (a.name_pretty < b.name_pretty);
            });

  VLOG(2) << __func__ << " : has regionlist: " << !regions_.empty();

  // If we can't get region list, we can't determine device region.
  if (regions_.empty())
    return false;
  return true;
}

void BraveVpnServiceDesktop::OnFetchTimezones(const std::string& timezones_list,
                                              bool success) {
  if (!success) {
    VLOG(2) << "Failed to get timezones list";
    SetFallbackDeviceRegion();
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(timezones_list);
  if (value && value->is_list()) {
    ParseAndCacheDeviceRegionName(std::move(*value));
    return;
  }

  SetFallbackDeviceRegion();
}

void BraveVpnServiceDesktop::ParseAndCacheDeviceRegionName(
    base::Value timezones_value) {
  DCHECK(timezones_value.is_list());

  if (!timezones_value.is_list()) {
    SetFallbackDeviceRegion();
    return;
  }

  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty()) {
    SetFallbackDeviceRegion();
    return;
  }

  for (const auto& timezones : timezones_value.GetList()) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict())
      continue;

    const std::string* region_name = timezones.FindStringKey("name");
    if (!region_name)
      continue;
    std::string default_region_name_candidate = *region_name;
    const base::Value* timezone_list_value = timezones.FindKey("timezones");
    if (!timezone_list_value || !timezone_list_value->is_list())
      continue;

    for (const auto& timezone : timezone_list_value->GetList()) {
      DCHECK(timezone.is_string());
      if (!timezone.is_string())
        continue;
      if (current_time_zone == timezone.GetString()) {
        SetDeviceRegion(*region_name);
        VLOG(2) << "Found default region: " << device_region_.name;
        return;
      }
    }
  }

  SetFallbackDeviceRegion();
}

void BraveVpnServiceDesktop::SetDeviceRegion(const std::string& name) {
  auto it =
      std::find_if(regions_.begin(), regions_.end(),
                   [&name](const auto& region) { return region.name == name; });
  if (it != regions_.end()) {
    SetDeviceRegion(*it);
  }
}

void BraveVpnServiceDesktop::SetFallbackDeviceRegion() {
  // Set first item in the region list as a |device_region_| as a fallback.
  DCHECK(!regions_.empty());
  if (regions_.empty())
    return;

  SetDeviceRegion(regions_[0]);
}

void BraveVpnServiceDesktop::SetDeviceRegion(
    const brave_vpn::mojom::Region& region) {
  device_region_ = region;

  DictionaryPrefUpdate update(prefs_, brave_vpn::prefs::kBraveVPNDeviceRegion);
  base::Value* dict = update.Get();
  dict->SetStringKey(kRegionContinentKey, device_region_.continent);
  dict->SetStringKey(kRegionNameKey, device_region_.name);
  dict->SetStringKey(kRegionNamePrettyKey, device_region_.name_pretty);
}

std::string BraveVpnServiceDesktop::GetCurrentTimeZone() {
  if (!test_timezone_.empty())
    return test_timezone_;

  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  icu::UnicodeString id;
  zone->getID(id);
  std::string current_time_zone;
  id.toUTF8String<std::string>(current_time_zone);
  return current_time_zone;
}

void BraveVpnServiceDesktop::GetAllRegions(GetAllRegionsCallback callback) {
  std::vector<brave_vpn::mojom::RegionPtr> regions;
  for (const auto& region : regions_) {
    regions.push_back(region.Clone());
  }
  std::move(callback).Run(std::move(regions));
}

void BraveVpnServiceDesktop::GetDeviceRegion(GetDeviceRegionCallback callback) {
  VLOG(2) << __func__;
  std::move(callback).Run(device_region_.Clone());
}

void BraveVpnServiceDesktop::GetSelectedRegion(
    GetSelectedRegionCallback callback) {
  VLOG(2) << __func__;

  if (!IsValidRegion(selected_region_)) {
    // Gives device region if there is no cached selected region.
    VLOG(2) << __func__ << " : give device region instead.";
    std::move(callback).Run(device_region_.Clone());
    return;
  }

  VLOG(2) << __func__ << " : Give " << selected_region_.name_pretty;
  std::move(callback).Run(selected_region_.Clone());
}

void BraveVpnServiceDesktop::SetSelectedRegion(
    brave_vpn::mojom::RegionPtr region_ptr) {
  VLOG(2) << __func__ << " : " << region_ptr->name_pretty;
  DictionaryPrefUpdate update(prefs_,
                              brave_vpn::prefs::kBraveVPNSelectedRegion);
  base::Value* dict = update.Get();
  dict->SetStringKey(kRegionContinentKey, region_ptr->continent);
  dict->SetStringKey(kRegionNameKey, region_ptr->name);
  dict->SetStringKey(kRegionNamePrettyKey, region_ptr->name_pretty);

  selected_region_.continent = region_ptr->continent;
  selected_region_.name = region_ptr->name;
  selected_region_.name_pretty = region_ptr->name_pretty;
}

void BraveVpnServiceDesktop::GetProductUrls(GetProductUrlsCallback callback) {
  brave_vpn::mojom::ProductUrls urls;
  urls.feedback = brave_vpn::kFeedbackUrl;
  urls.about = brave_vpn::kAboutUrl;
  urls.manage = brave_vpn::GetManageUrl();
  std::move(callback).Run(urls.Clone());
}

void BraveVpnServiceDesktop::FetchHostnamesForRegion(const std::string& name) {
  // Unretained is safe here becasue this class owns request helper.
  GetHostnamesForRegion(
      base::BindOnce(&BraveVpnServiceDesktop::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVpnServiceDesktop::OnFetchHostnames(const std::string& region,
                                              const std::string& hostnames,
                                              bool success) {
  if (cancel_connecting_) {
    cancel_connecting_ = false;
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, std::move(*value));
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnServiceDesktop::ParseAndCacheHostnames(
    const std::string& region,
    base::Value hostnames_value) {
  DCHECK(hostnames_value.is_list());
  if (!hostnames_value.is_list()) {
    VLOG(2) << __func__ << " : failed to parse hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  constexpr char kHostnameKey[] = "hostname";
  constexpr char kDisplayNameKey[] = "display-name";
  constexpr char kOfflineKey[] = "offline";
  constexpr char kCapacityScoreKey[] = "capacity-score";

  std::vector<brave_vpn::Hostname> hostnames;
  for (const auto& value : hostnames_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    const std::string* hostname_str = value.FindStringKey(kHostnameKey);
    const std::string* display_name_str = value.FindStringKey(kDisplayNameKey);
    absl::optional<bool> offline = value.FindBoolKey(kOfflineKey);
    absl::optional<int> capacity_score = value.FindIntKey(kCapacityScoreKey);

    if (!hostname_str || !display_name_str || !offline || !capacity_score)
      continue;

    hostnames.push_back(brave_vpn::Hostname{*hostname_str, *display_name_str,
                                            *offline, *capacity_score});
  }

  VLOG(2) << __func__ << " : has hostname: " << !hostnames.empty();

  if (hostnames.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  hostname_ = PickBestHostname(hostnames);
  if (hostname_.hostname.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : Picked " << hostname_.hostname << ", "
          << hostname_.display_name << ", " << hostname_.is_offline << ", "
          << hostname_.capacity_score;

  if (skus_credential_.empty()) {
    VLOG(2) << __func__ << " : skus_credential is empty";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  // Get subscriber credentials and then get EAP credentials with it to create
  // OS VPN entry.
  GetSubscriberCredentialV12(
      base::BindOnce(&BraveVpnServiceDesktop::OnGetSubscriberCredential,
                     base::Unretained(this)),
      GetBraveVPNPaymentsEnv(), skus_credential_);
}

void BraveVpnServiceDesktop::SetPurchasedState(PurchasedState state) {
  if (purchased_state_ == state)
    return;

  purchased_state_ = state;

  for (const auto& obs : observers_)
    obs->OnPurchasedStateChanged(purchased_state_);

  ScheduleFetchRegionDataIfNeeded();
}

void BraveVpnServiceDesktop::OnSkusVPNCredentialUpdated() {
  const std::string credential =
      prefs_->GetString(brave_rewards::prefs::kSkusVPNCredential);
  if (skus_credential_ == credential)
    return;

  skus_credential_ = credential;
  SetPurchasedState(PurchasedState::PURCHASED);
}

void BraveVpnServiceDesktop::OnGetSubscriberCredential(
    const std::string& subscriber_credential,
    bool success) {
  if (cancel_connecting_) {
    cancel_connecting_ = false;
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get subscriber credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received subscriber credential";

  GetProfileCredentials(
      base::BindOnce(&BraveVpnServiceDesktop::OnGetProfileCredentials,
                     base::Unretained(this)),
      subscriber_credential, hostname_.hostname);
}

void BraveVpnServiceDesktop::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  if (cancel_connecting_) {
    cancel_connecting_ = false;
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received profile credential";

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value && value->is_dict()) {
    constexpr char kUsernameKey[] = "eap-username";
    constexpr char kPasswordKey[] = "eap-password";
    const std::string* username = value->FindStringKey(kUsernameKey);
    const std::string* password = value->FindStringKey(kPasswordKey);
    if (!username || !password) {
      VLOG(2) << __func__ << " : it's invalid profile credential";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }

    connection_info_.SetConnectionInfo(kBraveVPNEntryName, hostname_.hostname,
                                       *username, *password);
    // Let's create os vpn entry with |connection_info_|.
    CreateVPNConnection();
    return;
  }

  VLOG(2) << __func__ << " : it's invalid profile credential";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

brave_vpn::Hostname BraveVpnServiceDesktop::PickBestHostname(
    const std::vector<brave_vpn::Hostname>& hostnames) {
  std::vector<brave_vpn::Hostname> filtered_hostnames;
  std::copy_if(
      hostnames.begin(), hostnames.end(),
      std::back_inserter(filtered_hostnames),
      [](const brave_vpn::Hostname& hostname) { return !hostname.is_offline; });

  std::sort(filtered_hostnames.begin(), filtered_hostnames.end(),
            [](const brave_vpn::Hostname& a, const brave_vpn::Hostname& b) {
              return a.capacity_score > b.capacity_score;
            });

  if (filtered_hostnames.empty())
    return brave_vpn::Hostname();

  // Pick highest capacity score.
  return filtered_hostnames[0];
}
