// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Growth
import OSLog
import Preferences
import Web

/// A P3A helper that will handle reporting dynamic P3A metrics around NTP SI interactions
///
/// A data source must be set in order to record events
final class NewTabPageP3AHelper {

  private let p3aUtils: BraveP3AUtils
  private let rewards: BraveRewards

  private var registrations: [P3ACallbackRegistration?] = []

  init(
    p3aUtils: BraveP3AUtils,
    rewards: BraveRewards
  ) {
    self.p3aUtils = p3aUtils
    self.rewards = rewards

    self.registrations.append(contentsOf: [
      self.p3aUtils.registerRotationCallback { [weak self] type in
        self?.rotated(type: type)
      },
      self.p3aUtils.registerMetricCycledCallback { [weak self] histogramName in
        self?.metricCycled(histogramName: histogramName)
      },
    ])
  }

  // MARK: - Record Events

  private var landingTimer: Timer?
  private var expectedLandingURL: URL?

  /// Records an NTP SI event which will be used to generate dynamic P3A metrics
  func recordEvent(
    _ event: EventType,
    on tab: some TabState,
    for sponsoredImage: NTPSponsoredImageBackground
  ) {
    if !p3aUtils.isP3AEnabled || rewards.isEnabled {
      return
    }
    let creativeInstanceId = sponsoredImage.creativeInstanceId
    updateMetricCount(creativeInstanceId: creativeInstanceId, event: event)
    if event == .tapped {
      expectedLandingURL = sponsoredImage.logo.destinationURL
      landingTimer?.invalidate()
      landingTimer = Timer.scheduledTimer(withTimeInterval: 5, repeats: false) {
        [weak self, weak tab] _ in
        guard let self = self, let tab = tab else { return }
        if let expectedURL = self.expectedLandingURL, expectedURL.isWebPage(),
          tab.visibleURL?.host == expectedURL.host
        {
          self.recordEvent(.landed, on: tab, for: sponsoredImage)
        }
      }
    }
  }

  private func updateMetricCount(
    creativeInstanceId: String,
    event: EventType
  ) {
    let name = DynamicHistogramName(creativeInstanceId: creativeInstanceId, eventType: event)

    p3aUtils.registerDynamicMetric(name.histogramName, logType: .express)

    var countsStorage = fetchEventsCountStorage()
    var eventCounts = countsStorage.eventCounts[name.creativeInstanceId, default: .init()]

    eventCounts.counts[name.eventType, default: 0] += 1

    countsStorage.eventCounts[name.creativeInstanceId] = eventCounts

    updateEventsCountStorage(countsStorage)
  }

  // MARK: - Storage

  private func fetchEventsCountStorage() -> Storage {
    let pref = Preferences.NewTabPage.sponsoredImageEventCount
    guard let json = pref.value, !json.isEmpty else {
      return .init()
    }
    do {
      return try JSONDecoder().decode(Storage.self, from: Data(json.utf8))
    } catch {
      Logger.module.error("Failed to decode NTP SI Event storage: \(error)")
      return .init()
    }
  }

  private func updateEventsCountStorage(_ storage: Storage) {
    let pref = Preferences.NewTabPage.sponsoredImageEventCount
    do {
      let json = String(data: try JSONEncoder().encode(storage), encoding: .utf8)
      pref.value = json
    } catch {
      Logger.module.error("Failed to encode NTP SI Event storage: \(error)")
    }
  }

  private func removeMetricIfInstanceDoesNotExist(name: DynamicHistogramName) {
    let eventType = name.eventType
    let creativeInstanceId = name.creativeInstanceId
    let storage = fetchEventsCountStorage()

    if let eventCounts = storage.eventCounts[creativeInstanceId],
      eventCounts.counts.keys.contains(where: { $0 == eventType })
    {
      return
    }
    p3aUtils.removeDynamicMetric(name.histogramName)
  }

  // MARK: - P3A Observers

  private func rotated(type: P3AMetricLogType) {
    if type != .express {
      return
    }
    if !p3aUtils.isP3AEnabled {
      Preferences.NewTabPage.sponsoredImageEventCount.value = nil
      return
    }

    let countBuckets: [Bucket] = [
      0,
      1,
      2,
      3,
      .r(4...8),
      .r(9...12),
      .r(13...16),
      .r(17...),
    ]

    var countsStorage = fetchEventsCountStorage()
    var totalActiveCreatives = 0
    for (creativeInstanceId, eventCounts) in countsStorage.eventCounts {
      for (eventType, count) in eventCounts.counts {
        let name = DynamicHistogramName(
          creativeInstanceId: creativeInstanceId,
          eventType: eventType
        )
        countsStorage.eventCounts[creativeInstanceId]?.inflightCounts[eventType] = count
        UmaHistogramRecordValueToBucket(name.histogramName, buckets: countBuckets, value: count)
      }
      if !eventCounts.counts.isEmpty {
        totalActiveCreatives += 1
      }
    }
    updateEventsCountStorage(countsStorage)

    let creativeTotalHistogramName = DynamicHistogramName(
      creativeInstanceId: "total",
      eventType: .init(rawValue: "count")
    ).histogramName
    // Always send the creative total if ads are disabled (as per spec),
    // or send the total if there were outstanding events sent
    if !rewards.isEnabled || totalActiveCreatives > 0 {
      p3aUtils.registerDynamicMetric(creativeTotalHistogramName, logType: .express)
      UmaHistogramRecordValueToBucket(
        creativeTotalHistogramName,
        buckets: countBuckets,
        value: totalActiveCreatives
      )
    }
  }

  private func metricCycled(histogramName: String) {
    guard let name = DynamicHistogramName(computedHistogramName: histogramName) else {
      return
    }
    defer {
      removeMetricIfInstanceDoesNotExist(name: name)
    }
    var countsStorage = fetchEventsCountStorage()
    guard var eventCounts = countsStorage.eventCounts[name.creativeInstanceId] else {
      return
    }
    let fullCount = eventCounts.counts[name.eventType] ?? 0
    let inflightCount = eventCounts.inflightCounts[name.eventType] ?? 0
    let newCount = fullCount - inflightCount

    eventCounts.inflightCounts.removeValue(forKey: name.eventType)

    if newCount > 0 {
      eventCounts.counts[name.eventType] = newCount
    } else {
      eventCounts.counts.removeValue(forKey: name.eventType)
      if eventCounts.counts.isEmpty {
        countsStorage.eventCounts.removeValue(forKey: name.creativeInstanceId)
      }
    }

    if countsStorage.eventCounts[name.creativeInstanceId] != nil {
      countsStorage.eventCounts[name.creativeInstanceId] = eventCounts
    }

    updateEventsCountStorage(countsStorage)
  }

  // MARK: -

  struct EventType: RawRepresentable, Hashable, Codable {
    var rawValue: String

    static let viewed: Self = .init(rawValue: "views")
    static let tapped: Self = .init(rawValue: "clicks")
    static let landed: Self = .init(rawValue: "lands")
    static let mediaPlay: Self = .init(rawValue: "media_play")
    static let media25: Self = .init(rawValue: "media_25")
    static let media100: Self = .init(rawValue: "media_100")
  }

  struct Storage: Codable {
    typealias CreativeInstanceID = String

    struct EventCounts: Codable {
      var inflightCounts: [EventType: Int] = [:]
      var counts: [EventType: Int] = [:]
    }

    var eventCounts: [CreativeInstanceID: EventCounts]

    init(eventCounts: [CreativeInstanceID: EventCounts] = [:]) {
      self.eventCounts = eventCounts
    }

    func encode(to encoder: Encoder) throws {
      var container = encoder.singleValueContainer()
      try container.encode(self.eventCounts)
    }

    init(from decoder: Decoder) throws {
      let container = try decoder.singleValueContainer()
      self.eventCounts = try container.decode([String: EventCounts].self)
    }
  }

  private struct DynamicHistogramName: CustomStringConvertible {
    var creativeInstanceId: String
    var eventType: EventType

    init(creativeInstanceId: String, eventType: EventType) {
      self.creativeInstanceId = creativeInstanceId
      self.eventType = eventType
    }

    init?(computedHistogramName: String) {
      if !computedHistogramName.hasPrefix(P3ACreativeMetricPrefix) {
        return nil
      }
      let items = computedHistogramName.split(separator: ".").map(String.init)
      if items.count != 3 {
        // The histogram name provided should be created from `histogramName` below
        return nil
      }
      self.creativeInstanceId = items[1]
      self.eventType = EventType(rawValue: items[2])
    }

    var histogramName: String {
      // `P3ACreativeMetricPrefix` contains a trailing dot
      return "\(P3ACreativeMetricPrefix)\(creativeInstanceId).\(eventType.rawValue)"
    }

    var description: String {
      histogramName
    }
  }
}
