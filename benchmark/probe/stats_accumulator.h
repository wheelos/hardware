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

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace apollo {
namespace drivers {
namespace benchmark {

struct LatencySummary {
  uint64_t count = 0;
  double mean_ms = 0.0;
  double p50_ms = 0.0;
  double p95_ms = 0.0;
  double p99_ms = 0.0;
  double max_ms = 0.0;
};

struct ChannelBenchmarkSummary {
  uint64_t message_count = 0;
  uint64_t total_payload_bytes = 0;
  uint64_t dropped_sequence_count = 0;
  double elapsed_sec = 0.0;
  double hz = 0.0;
  double mean_mbps = 0.0;
  double peak_window_mbps = 0.0;
  LatencySummary driver_to_app_ms;
  LatencySummary hw_to_driver_ms;
  std::string transport_mode;
  std::string channel;
  std::string msg_type;
};

class StatsAccumulator {
 public:
  explicit StatsAccumulator(uint64_t window_ns = 100000000ULL)
      : window_ns_(window_ns > 0 ? window_ns : 100000000ULL) {}

  void RecordSample(uint64_t arrival_mono_ns, uint64_t payload_bytes,
                    bool has_seq, uint32_t seq_num,
                    bool has_driver_to_app, double driver_to_app_ms,
                    bool has_hw_to_driver, double hw_to_driver_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (first_arrival_ns_ == 0) {
      first_arrival_ns_ = arrival_mono_ns;
      current_window_start_ns_ = arrival_mono_ns;
    }
    last_arrival_ns_ = arrival_mono_ns;
    ++message_count_;
    total_payload_bytes_ += payload_bytes;

    // Track 100ms tumbling window peak bandwidth.
    if (arrival_mono_ns < current_window_start_ns_ + window_ns_) {
      current_window_bytes_ += payload_bytes;
    } else {
      FlushWindowLocked();
      current_window_start_ns_ = arrival_mono_ns;
      current_window_bytes_ = payload_bytes;
    }

    if (has_seq && seq_num > 0) {
      if (has_last_seq_ && seq_num > last_seq_ + 1) {
        dropped_sequence_count_ += static_cast<uint64_t>(seq_num - last_seq_ - 1);
      }
      last_seq_ = seq_num;
      has_last_seq_ = true;
    }

    if (has_driver_to_app && std::isfinite(driver_to_app_ms) &&
        driver_to_app_ms >= 0.0 && driver_to_app_ms < 60000.0) {
      driver_to_app_samples_ms_.push_back(driver_to_app_ms);
    }

    if (has_hw_to_driver && std::isfinite(hw_to_driver_ms) &&
        hw_to_driver_ms >= -1000.0 && hw_to_driver_ms < 60000.0) {
      hw_to_driver_samples_ms_.push_back(hw_to_driver_ms);
    }
  }

  ChannelBenchmarkSummary Finalize(double wall_elapsed_sec,
                                   const std::string& channel,
                                   const std::string& msg_type,
                                   const std::string& transport_mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    FlushWindowLocked();

    ChannelBenchmarkSummary summary;
    summary.channel = channel;
    summary.msg_type = msg_type;
    summary.transport_mode = transport_mode;
    summary.message_count = message_count_;
    summary.total_payload_bytes = total_payload_bytes_;
    summary.dropped_sequence_count = dropped_sequence_count_;

    double effective_sec = wall_elapsed_sec;
    if (effective_sec <= 0.0 && last_arrival_ns_ > first_arrival_ns_) {
      effective_sec =
          static_cast<double>(last_arrival_ns_ - first_arrival_ns_) / 1e9;
    }
    summary.elapsed_sec = effective_sec;
    if (effective_sec > 0.0) {
      summary.hz = static_cast<double>(message_count_) / effective_sec;
      summary.mean_mbps =
          (static_cast<double>(total_payload_bytes_) * 8.0) /
          (effective_sec * 1e6);
    }
    summary.peak_window_mbps = peak_window_mbps_;
    summary.driver_to_app_ms = Summarize(driver_to_app_samples_ms_);
    summary.hw_to_driver_ms = Summarize(hw_to_driver_samples_ms_);
    return summary;
  }

  static std::string ToJson(const ChannelBenchmarkSummary& s) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "{\n";
    oss << "  \"channel\": \"" << EscapeJson(s.channel) << "\",\n";
    oss << "  \"msg_type\": \"" << EscapeJson(s.msg_type) << "\",\n";
    oss << "  \"latency_timestamp\": \"callback_entry\",\n";
    oss << "  \"transport_mode\": \"" << EscapeJson(s.transport_mode)
        << "\",\n";
    oss << "  \"message_count\": " << s.message_count << ",\n";
    oss << "  \"total_payload_bytes\": " << s.total_payload_bytes << ",\n";
    oss << "  \"dropped_sequence_count\": " << s.dropped_sequence_count
        << ",\n";
    oss << "  \"elapsed_sec\": " << s.elapsed_sec << ",\n";
    oss << "  \"hz\": " << s.hz << ",\n";
    oss << "  \"mean_mbps\": " << s.mean_mbps << ",\n";
    oss << "  \"peak_window_mbps\": " << s.peak_window_mbps << ",\n";
    oss << "  \"driver_to_app_ms\": " << LatencyToJson(s.driver_to_app_ms)
        << ",\n";
    oss << "  \"hw_to_driver_ms\": " << LatencyToJson(s.hw_to_driver_ms)
        << "\n";
    oss << "}\n";
    return oss.str();
  }

 private:
  void FlushWindowLocked() {
    if (current_window_bytes_ == 0) {
      return;
    }
    const double mbps =
        (static_cast<double>(current_window_bytes_) * 8.0) /
        (static_cast<double>(window_ns_) / 1e9 * 1e6);
    if (mbps > peak_window_mbps_) {
      peak_window_mbps_ = mbps;
    }
    current_window_bytes_ = 0;
  }

  static LatencySummary Summarize(std::vector<double> values) {
    LatencySummary out;
    if (values.empty()) {
      return out;
    }
    std::sort(values.begin(), values.end());
    out.count = values.size();
    double sum = 0.0;
    for (double v : values) {
      sum += v;
    }
    out.mean_ms = sum / static_cast<double>(values.size());
    out.p50_ms = Percentile(values, 0.50);
    out.p95_ms = Percentile(values, 0.95);
    out.p99_ms = Percentile(values, 0.99);
    out.max_ms = values.back();
    return out;
  }

  static double Percentile(const std::vector<double>& sorted, double q) {
    if (sorted.empty()) {
      return 0.0;
    }
    const double idx = q * static_cast<double>(sorted.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    if (lo == hi) {
      return sorted[lo];
    }
    const double frac = idx - static_cast<double>(lo);
    return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
  }

  static std::string LatencyToJson(const LatencySummary& l) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "{"
        << "\"count\": " << l.count << ", "
        << "\"mean_ms\": " << l.mean_ms << ", "
        << "\"p50_ms\": " << l.p50_ms << ", "
        << "\"p95_ms\": " << l.p95_ms << ", "
        << "\"p99_ms\": " << l.p99_ms << ", "
        << "\"max_ms\": " << l.max_ms << "}";
    return oss.str();
  }

  static std::string EscapeJson(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
      if (c == '\"') {
        oss << "\\\"";
      } else if (c == '\\') {
        oss << "\\\\";
      } else {
        oss << c;
      }
    }
    return oss.str();
  }

  const uint64_t window_ns_;
  std::mutex mutex_;
  uint64_t first_arrival_ns_ = 0;
  uint64_t last_arrival_ns_ = 0;
  uint64_t current_window_start_ns_ = 0;
  uint64_t current_window_bytes_ = 0;
  double peak_window_mbps_ = 0.0;
  uint64_t message_count_ = 0;
  uint64_t total_payload_bytes_ = 0;
  uint64_t dropped_sequence_count_ = 0;
  uint32_t last_seq_ = 0;
  bool has_last_seq_ = false;
  std::vector<double> driver_to_app_samples_ms_;
  std::vector<double> hw_to_driver_samples_ms_;
};

}  // namespace benchmark
}  // namespace drivers
}  // namespace apollo
