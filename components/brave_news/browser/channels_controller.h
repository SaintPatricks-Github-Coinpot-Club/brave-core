// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_

#include <array>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

class SubscriptionsSnapshot;

namespace p3a {
class NewsMetrics;
}  // namespace p3a

using Channels = base::flat_map<std::string, mojom::ChannelPtr>;
using ChannelsCallback = base::OnceCallback<void(Channels)>;

inline constexpr char kTopSourcesChannel[] = "Top Sources";
inline constexpr char kTopNewsChannel[] = "Top News";
// Channels that are considered sensitive and should not be included in the feed
// unless the user explicitly subscribes to them.
inline constexpr std::array<const char*, 1> kSensitiveChannels = {"Politics"};

// Get all the channels for a publisher in a specific locale.
std::vector<std::string> GetChannelsForPublisher(
    const std::string& locale,
    const mojom::PublisherPtr& publisher);

class ChannelsController {
 public:
  explicit ChannelsController(PublishersController* publishers_controller);
  ~ChannelsController();
  ChannelsController(const ChannelsController&) = delete;
  ChannelsController& operator=(const ChannelsController&) = delete;

  static Channels GetChannelsFromPublishers(
      const Publishers& publishers,
      const SubscriptionsSnapshot& subscriptions);

  void GetAllChannels(const SubscriptionsSnapshot& subscriptions,
                      ChannelsCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveNewsFeedBuildingTest, BuildFeed);
  FRIEND_TEST_ALL_PREFIXES(BraveNewsFeedBuildingTest,
                           DuplicateItemsAreNotIncluded);
  raw_ptr<PublishersController> publishers_controller_;
};
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_
