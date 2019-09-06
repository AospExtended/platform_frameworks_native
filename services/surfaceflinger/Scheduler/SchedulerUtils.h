/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <chrono>
#include <cinttypes>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace android::scheduler {

// Opaque handle to scheduler connection.
struct ConnectionHandle {
    using Id = std::uintptr_t;
    static constexpr Id INVALID_ID = static_cast<Id>(-1);

    Id id = INVALID_ID;

    explicit operator bool() const { return id != INVALID_ID; }
};

inline bool operator==(ConnectionHandle lhs, ConnectionHandle rhs) {
    return lhs.id == rhs.id;
}

using namespace std::chrono_literals;

// This number is used to have a place holder for when the screen is not NORMAL/ON. Currently
// the config is not visible to SF, and is completely maintained by HWC. However, we would
// still like to keep track of time when the device is in this config.
static constexpr int SCREEN_OFF_CONFIG_ID = -1;
static constexpr uint32_t HWC2_SCREEN_OFF_CONFIG_ID = 0xffffffff;

// This number is used when we try to determine how long do we keep layer information around
// before we remove it. It is also used to determine how long the layer stays relevant.
// This time period captures infrequent updates when playing YouTube video with static image,
// or waiting idle in messaging app, when cursor is blinking.
static constexpr std::chrono::nanoseconds OBSOLETE_TIME_EPSILON_NS = 1200ms;

// Layer is considered low activity if the buffers come more than LOW_ACTIVITY_EPSILON_NS
// apart. This is helping SF to vote for lower refresh rates when there is not activity
// in screen.
static constexpr std::chrono::nanoseconds LOW_ACTIVITY_EPSILON_NS = 250ms;

// Calculates the statistical mean (average) in the data structure (array, vector). The
// function does not modify the contents of the array.
template <typename T>
auto calculate_mean(const T& v) {
    using V = typename T::value_type;
    V sum = std::accumulate(v.begin(), v.end(), static_cast<V>(0));
    return sum / static_cast<V>(v.size());
}

// Calculates the statistical median in the vector. Return 0 if the vector is empty. The
// function modifies the vector contents.
int64_t calculate_median(std::vector<int64_t>* v);

// Calculates the statistical mode in the vector. Return 0 if the vector is empty.
template <typename T>
auto calculate_mode(const T& v) {
    if (v.empty()) {
        return 0;
    }

    // Create a map with all the counts for the indivicual values in the vector.
    std::unordered_map<int64_t, int> counts;
    for (int64_t value : v) {
        counts[value]++;
    }

    // Sort the map, and return the number with the highest count. If two numbers have
    // the same count, first one is returned.
    using ValueType = const decltype(counts)::value_type&;
    const auto compareCounts = [](ValueType l, ValueType r) { return l.second <= r.second; };
    return static_cast<int>(std::max_element(counts.begin(), counts.end(), compareCounts)->first);
}

} // namespace android::scheduler

namespace std {

template <>
struct hash<android::scheduler::ConnectionHandle> {
    size_t operator()(android::scheduler::ConnectionHandle handle) const {
        return hash<android::scheduler::ConnectionHandle::Id>()(handle.id);
    }
};

} // namespace std