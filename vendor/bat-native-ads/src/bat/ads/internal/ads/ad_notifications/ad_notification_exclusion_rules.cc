/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification_exclusion_rules.h"

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace ad_notifications {
namespace frequency_capping {

ExclusionRules::ExclusionRules(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const AdEventList& ad_events,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(subdivision_targeting,
                         anti_targeting_resource,
                         ad_events,
                         browsing_history) {}

ExclusionRules::~ExclusionRules() = default;

bool ExclusionRules::ShouldExcludeCreativeAd(
    const CreativeAdInfo& creative_ad) {
  if (ExclusionRulesBase::ShouldExcludeCreativeAd(creative_ad)) {
    return true;
  }

  DismissedFrequencyCap dismissed_frequency_cap(ad_events_);
  if (AddToCacheIfNeeded(creative_ad, &dismissed_frequency_cap)) {
    return true;
  }

  return false;
}

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads
