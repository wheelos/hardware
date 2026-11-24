// Copyright 2026 The WheelOS Team. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace apollo {
namespace drivers {
namespace benchmark {

struct ArrivalSample {
  uint64_t monotonic_ns;
  uint64_t realtime_ns;
  uint64_t payload_bytes;
};

template <typename MonoClock, typename RealClock, typename MeasureSize>
ArrivalSample CaptureArrival(MonoClock monotonic, RealClock realtime,
                             MeasureSize measure_size) {
  const uint64_t monotonic_ns = monotonic();
  const uint64_t realtime_ns = realtime();
  return {monotonic_ns, realtime_ns, measure_size()};
}

}  // namespace benchmark
}  // namespace drivers
}  // namespace apollo
