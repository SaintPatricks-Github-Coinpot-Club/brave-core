/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_

#include <set>
#include <string>

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

struct CreativeAdInfo;

class ExclusionRulesBase {
 public:
  ExclusionRulesBase(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);
  virtual ~ExclusionRulesBase();

  virtual bool ShouldExcludeCreativeAd(const CreativeAdInfo& creative_ad);

 protected:
  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;                         // NOT OWNED
  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED
  AdEventList ad_events_;
  BrowsingHistoryList browsing_history_;

  std::set<std::string> uuids_;
  bool AddToCacheIfNeeded(const CreativeAdInfo& creative_ad,
                          ExclusionRule<CreativeAdInfo>* exclusion_rule);

 private:
  bool IsCached(const CreativeAdInfo& creative_ad) const;
  void AddToCache(const std::string& uuid);

  ExclusionRulesBase(const ExclusionRulesBase&) = delete;
  ExclusionRulesBase& operator=(const ExclusionRulesBase&) = delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_
