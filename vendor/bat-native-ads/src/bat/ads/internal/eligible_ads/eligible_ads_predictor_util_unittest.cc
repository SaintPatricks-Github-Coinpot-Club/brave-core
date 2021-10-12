/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_predictor_util.h"

#include "base/guid.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_features.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsEligibleAdsPredictorUtilTest, GroupCreativeAdsByCreativeInstanceId) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar1");
  creative_ads.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar2");
  creative_ads.push_back(creative_ad_notification_2);

  const CreativeAdNotificationInfo creative_ad_notification_3 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar3");
  creative_ads.push_back(creative_ad_notification_3);

  const CreativeAdNotificationInfo creative_ad_notification_4 =
      GetCreativeAdNotification(creative_ad_notification_2.creative_instance_id,
                                "foo-bar4");
  creative_ads.push_back(creative_ad_notification_4);

  // Act
  const CreativeAdPredictorMap<CreativeAdNotificationInfo>
      creative_ad_predictors =
          GroupCreativeAdsByCreativeInstanceId(creative_ads);

  // Assert
  ASSERT_EQ(3U, creative_ad_predictors.size());

  const AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor =
      creative_ad_predictors.at(
          creative_ad_notification_2.creative_instance_id);
  const SegmentList& expected_segments = {"foo-bar2", "foo-bar4"};
  EXPECT_EQ(expected_segments, ad_predictor.segments);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     GroupCreativeAdsByCreativeInstanceIdForEmptyAds) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  // Act
  const CreativeAdPredictorMap<CreativeAdNotificationInfo>
      creative_ad_predictors =
          GroupCreativeAdsByCreativeInstanceId(creative_ads);

  // Assert
  EXPECT_TRUE(creative_ad_predictors.empty());
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithZeroWeightsNotAllowedByGriffin) {
  // Arrange
  const char kAdFeatureWeights[] = "ad_predictor_weights";
  std::map<std::string, std::string> kEligibleAdsParameters;
  kEligibleAdsParameters[kAdFeatureWeights] =
      "0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kEligibleAds, kEligibleAdsParameters}}, {});

  const std::string segment = "foo-bar";
  const double ptr = 1.0;
  const double priority = 1;
  const CreativeAdNotificationInfo creative_ad =
      GetCreativeAdNotification(segment, ptr, priority);

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor;
  ad_predictor.creative_ad = creative_ad;
  ad_predictor.segments = {segment};
  ad_predictor.does_match_intent_child_segments = 1;
  ad_predictor.does_match_intent_parent_segments = 0;
  ad_predictor.does_match_interest_child_segments = 0;
  ad_predictor.does_match_interest_parent_segments = 0;
  ad_predictor.ad_last_seen_hours_ago = 15;
  ad_predictor.advertiser_last_seen_hours_ago = 48;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  EXPECT_LT(0, ad_predictor.score);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithDefaultWeights) {
  // Arrange
  const std::string segment = "foo-bar";
  const double ptr = 1.0;
  const double priority = 1;
  const CreativeAdNotificationInfo creative_ad =
      GetCreativeAdNotification(segment, ptr, priority);

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor;
  ad_predictor.creative_ad = creative_ad;
  ad_predictor.segments = {segment};
  ad_predictor.does_match_intent_child_segments = 1;
  ad_predictor.does_match_intent_parent_segments = 0;
  ad_predictor.does_match_interest_child_segments = 0;
  ad_predictor.does_match_interest_parent_segments = 0;
  ad_predictor.ad_last_seen_hours_ago = 15;
  ad_predictor.advertiser_last_seen_hours_ago = 48;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  const double expected_score = 0.0 + 1.0 + 1.0 * (15 / 24.0) + 1.0 * 1.0;
  EXPECT_EQ(expected_score, ad_predictor.score);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithEmptyAdFeatures) {
  // Arrange
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  const double expected_score = 0.0;
  EXPECT_EQ(expected_score, ad_predictor.score);
}

}  // namespace ads
