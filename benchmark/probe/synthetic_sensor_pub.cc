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

#include <time.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gflags/gflags.h"

#include "cyber/cyber.h"
#include "wheelos_msgs/sensor_msgs/sensor_image.pb.h"

DEFINE_string(channel, "/apollo/sensor/bench/synthetic/image",
              "Target Cyber channel.");
DEFINE_double(hz, 20.0, "Publish rate in Hz.");
DEFINE_int32(payload_bytes, 65536, "Synthetic payload bytes per frame.");
DEFINE_double(duration_sec, 6.0, "Total publish duration in seconds.");
DEFINE_bool(simulate_copy, true,
            "Perform an explicit user-space memcpy into the protobuf payload.");

namespace {
std::atomic<bool> g_stop{false};

void OnSignal(int /*sig*/) { g_stop.store(true); }

double RealtimeSec() {
  timespec ts{};
  clock_gettime(CLOCK_REALTIME, &ts);
  return static_cast<double>(ts.tv_sec) +
         static_cast<double>(ts.tv_nsec) * 1e-9;
}
}  // namespace

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::signal(SIGINT, OnSignal);
  std::signal(SIGTERM, OnSignal);

  if (!apollo::cyber::Init(argv[0])) {
    return 1;
  }
  auto node = apollo::cyber::CreateNode("synthetic_sensor_pub");
  auto writer = node->CreateWriter<apollo::drivers::Image>(FLAGS_channel);
  if (!writer) {
    apollo::cyber::Clear();
    return 1;
  }

  const size_t nbytes = static_cast<size_t>(std::max(1024, FLAGS_payload_bytes));
  std::vector<uint8_t> staging(nbytes, 0x5A);
  const auto period = std::chrono::microseconds(
      static_cast<int64_t>(1e6 / std::max(1.0, FLAGS_hz)));
  const auto start = std::chrono::steady_clock::now();
  uint32_t seq = 1;

  while (!g_stop.load() && !apollo::cyber::IsShutdown()) {
    const double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();
    if (FLAGS_duration_sec > 0.0 && elapsed >= FLAGS_duration_sec) {
      break;
    }

    auto msg = std::make_shared<apollo::drivers::Image>();
    msg->set_width(256);
    msg->set_height(static_cast<uint32_t>(nbytes / 256));
    msg->set_encoding("mono8");
    msg->set_step(256);
    msg->mutable_data()->resize(nbytes);
    if (FLAGS_simulate_copy) {
      std::memcpy(msg->mutable_data()->data(), staging.data(), nbytes);
    }

    const double now_sec = RealtimeSec();
    msg->set_measurement_time(now_sec - 0.002);
    auto* header = msg->mutable_header();
    header->set_module_name("synthetic_sensor");
    header->set_sequence_num(seq++);
    header->set_timestamp_sec(now_sec);

    writer->Write(msg);
    std::this_thread::sleep_for(period);
  }

  apollo::cyber::Clear();
  return 0;
}
