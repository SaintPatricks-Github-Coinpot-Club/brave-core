/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

TotalMaxFrequencyCap::TotalMaxFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

TotalMaxFrequencyCap::~TotalMaxFrequencyCap() = default;

std::string TotalMaxFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool TotalMaxFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  const AdEventList filtered_ad_events =
      FilterAdEvents(ad_events_, creative_ad);

  if (!DoesRespectCap(filtered_ad_events, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the frequency capping for totalMax",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string TotalMaxFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool TotalMaxFrequencyCap::DoesRespectCap(const AdEventList& ad_events,
                                          const CreativeAdInfo& creative_ad) {
  if (ad_events.size() >= creative_ad.total_max) {
    return false;
  }

  return true;
}

AdEventList TotalMaxFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(
      filtered_ad_events.begin(), filtered_ad_events.end(),
      [&creative_ad](const AdEventInfo& ad_event) {
        return !DoesAdTypeSupportFrequencyCapping(ad_event.type) ||
               ad_event.creative_set_id != creative_ad.creative_set_id ||
               ad_event.confirmation_type != ConfirmationType::kServed;
      });

  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
