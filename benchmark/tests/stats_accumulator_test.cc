// Copyright 2026 The WheelOS Team. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "modules/drivers/benchmark/stats_accumulator.h"

#include "gtest/gtest.h"

namespace apollo {
namespace drivers {
namespace benchmark {

TEST(StatsAccumulatorTest, ComputesPercentilesDropsAndPeakWindow) {
  StatsAccumulator acc(100000000ULL);  // 100ms window

  // 5 samples in window 0 (t=0..80ms): seq 1, 2, 4 (1 dropped), 5, 6
  const uint32_t seqs[5] = {1, 2, 4, 5, 6};
  const double d2a[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
  for (int i = 0; i < 5; ++i) {
    acc.RecordSample(1000000000ULL + static_cast<uint64_t>(i) * 20000000ULL,
                     100000ULL, true, seqs[i], true, d2a[i], true, 2.0);
  }
  // 1 sample in next window -> flushes window 0 (500 KB in 0.1s = 40 Mbps).
  acc.RecordSample(1200000000ULL, 100000ULL, true, 7, true, 10.0, true, 4.0);

  const auto summary = acc.Finalize(1.0, "/sensor/test", "pointcloud", "SHM");
  EXPECT_EQ(summary.message_count, 6U);
  EXPECT_EQ(summary.total_payload_bytes, 600000U);
  EXPECT_EQ(summary.dropped_sequence_count, 1U);
  EXPECT_NEAR(summary.hz, 6.0, 1e-6);
  EXPECT_NEAR(summary.peak_window_mbps, 40.0, 1e-3);
  EXPECT_NEAR(summary.driver_to_app_ms.p50_ms, 3.5, 1e-3);
  EXPECT_NEAR(summary.driver_to_app_ms.max_ms, 10.0, 1e-6);
  EXPECT_NEAR(summary.hw_to_driver_ms.p50_ms, 2.0, 1e-6);

  const std::string json = StatsAccumulator::ToJson(summary);
  EXPECT_NE(json.find("\"transport_mode\": \"SHM\""), std::string::npos);
  EXPECT_NE(json.find("\"dropped_sequence_count\": 1"), std::string::npos);
}

}  // namespace benchmark
}  // namespace drivers
}  // namespace apollo
