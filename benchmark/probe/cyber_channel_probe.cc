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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "gflags/gflags.h"

#include "cyber/common/global_data.h"
#include "cyber/cyber.h"
#include "cyber/message/raw_message.h"
#include "cyber/proto/transport_conf.pb.h"
#include "modules/drivers/benchmark/arrival_sample.h"
#include "modules/drivers/benchmark/header_envelope.pb.h"
#include "modules/drivers/benchmark/stats_accumulator.h"
#include "wheelos_msgs/sensor_msgs/imu.pb.h"
#include "wheelos_msgs/sensor_msgs/pointcloud.pb.h"
#include "wheelos_msgs/sensor_msgs/sensor_image.pb.h"

DEFINE_string(channel, "", "Cyber channel to subscribe for benchmarking.");
DEFINE_string(msg_type, "auto",
              "Message type: auto|raw|pointcloud|image|compressed_image|imu.");
DEFINE_double(warmup_sec, 0.0, "Warmup seconds before recording samples.");
DEFINE_double(duration_sec, 10.0,
              "Measurement duration in seconds after warmup (0 = until signal).");
DEFINE_string(output_json, "", "Path to write benchmark summary JSON.");
DEFINE_string(node_name, "wheelos_sensor_bench_probe", "Cyber node name.");
DEFINE_string(window_file, "", "Shared monotonic measurement window.");
DEFINE_string(ready_file, "", "Path to signal that the reader is ready.");

namespace {

std::atomic<bool> g_stop_requested{false};

void HandleSignal(int /*sig*/) { g_stop_requested.store(true); }

uint64_t MonoNowNs() {
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
         static_cast<uint64_t>(ts.tv_nsec);
}

uint64_t RealtimeNowNs() {
  timespec ts{};
  clock_gettime(CLOCK_REALTIME, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
         static_cast<uint64_t>(ts.tv_nsec);
}

std::string TransportModeName() {
  using apollo::cyber::proto::OptionalMode;
  const auto& transport_conf =
      apollo::cyber::common::GlobalData::Instance()->Config().transport_conf();
  switch (transport_conf.communication_mode().same_proc()) {
    default:
      break;
  }
  const auto diff_proc = transport_conf.communication_mode().diff_proc();
  switch (diff_proc) {
    case OptionalMode::INTRA:
      return "INTRA";
    case OptionalMode::SHM:
      return "SHM";
    case OptionalMode::RTPS:
      return "RTPS";
    case OptionalMode::ICEORYX:
      return "ICEORYX";
    case OptionalMode::HYBRID:
    default:
      return "HYBRID";
  }
}

void ComputeHeaderLatencies(const apollo::common::Header& header,
                            bool has_measurement_time,
                            double measurement_time_sec,
                            uint64_t recv_unix_ns, bool* has_seq,
                            uint32_t* seq_num, bool* has_driver_to_app,
                            double* driver_to_app_ms, bool* has_hw_to_driver,
                            double* hw_to_driver_ms) {
  *has_seq = header.has_sequence_num();
  *seq_num = header.sequence_num();
  *has_driver_to_app = false;
  *driver_to_app_ms = 0.0;
  *has_hw_to_driver = false;
  *hw_to_driver_ms = 0.0;

  if (header.has_timestamp_sec() && header.timestamp_sec() > 0.0) {
    const double pub_ns = header.timestamp_sec() * 1e9;
    *driver_to_app_ms = (static_cast<double>(recv_unix_ns) - pub_ns) / 1e6;
    *has_driver_to_app = true;

    if (has_measurement_time && measurement_time_sec > 0.0) {
      *hw_to_driver_ms =
          (header.timestamp_sec() - measurement_time_sec) * 1000.0;
      *has_hw_to_driver = true;
    } else if (header.has_lidar_timestamp() && header.lidar_timestamp() > 0) {
      *hw_to_driver_ms =
          (pub_ns - static_cast<double>(header.lidar_timestamp())) / 1e6;
      *has_hw_to_driver = true;
    } else if (header.has_camera_timestamp() && header.camera_timestamp() > 0) {
      *hw_to_driver_ms =
          (pub_ns - static_cast<double>(header.camera_timestamp())) / 1e6;
      *has_hw_to_driver = true;
    } else if (header.has_radar_timestamp() && header.radar_timestamp() > 0) {
      *hw_to_driver_ms =
          (pub_ns - static_cast<double>(header.radar_timestamp())) / 1e6;
      *has_hw_to_driver = true;
    }
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_channel.empty()) {
    std::cerr << "--channel is required\n";
    return 2;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  if (!apollo::cyber::Init(argv[0])) {
    std::cerr << "Failed to initialize Cyber RT\n";
    return 1;
  }

  auto node = apollo::cyber::CreateNode(FLAGS_node_name);
  if (!node) {
    std::cerr << "Failed to create Cyber node\n";
    apollo::cyber::Clear();
    return 1;
  }

  apollo::drivers::benchmark::StatsAccumulator acc;
  const uint64_t start_mono_ns = MonoNowNs();
  const uint64_t warmup_ns =
      static_cast<uint64_t>(std::max(0.0, FLAGS_warmup_sec) * 1e9);
  const uint64_t measure_start_ns = start_mono_ns + warmup_ns;
  const uint64_t* window = nullptr;
  if (!FLAGS_window_file.empty()) {
    const int fd = open(FLAGS_window_file.c_str(), O_RDONLY);
    struct stat st {};
    if (fd < 0 || fstat(fd, &st) != 0 ||
        st.st_size < static_cast<off_t>(2 * sizeof(uint64_t))) {
      if (fd >= 0) close(fd);
      std::cerr << "Failed to open measurement window\n";
      return 1;
    }
    void* mapping =
        mmap(nullptr, 2 * sizeof(uint64_t), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (mapping == MAP_FAILED) {
      std::cerr << "Failed to map measurement window\n";
      return 1;
    }
    window = static_cast<const uint64_t*>(mapping);
  }

  auto record_header_msg = [&](uint64_t arrival_mono_ns, uint64_t recv_unix_ns,
                               uint64_t payload_bytes,
                               const apollo::common::Header* header,
                               bool has_measurement_time,
                               double measurement_time_sec) {
    const uint64_t start = window
        ? __atomic_load_n(&window[0], __ATOMIC_ACQUIRE) : measure_start_ns;
    const uint64_t end = window
        ? __atomic_load_n(&window[1], __ATOMIC_RELAXED) : UINT64_MAX;
    if (start == 0 || arrival_mono_ns < start || arrival_mono_ns >= end) {
      return;
    }
    bool has_seq = false;
    uint32_t seq_num = 0;
    bool has_d2a = false;
    double d2a_ms = 0.0;
    bool has_hw2d = false;
    double hw2d_ms = 0.0;
    if (header != nullptr) {
      ComputeHeaderLatencies(*header, has_measurement_time,
                             measurement_time_sec, recv_unix_ns, &has_seq,
                             &seq_num, &has_d2a, &d2a_ms, &has_hw2d, &hw2d_ms);
    }
    acc.RecordSample(arrival_mono_ns, payload_bytes, has_seq, seq_num, has_d2a,
                     d2a_ms, has_hw2d, hw2d_ms);
  };
  auto record_typed_msg = [&](const auto& msg) {
    const auto sample = apollo::drivers::benchmark::CaptureArrival(
        MonoNowNs, RealtimeNowNs, [&]() { return msg->ByteSizeLong(); });
    record_header_msg(sample.monotonic_ns, sample.realtime_ns, sample.payload_bytes,
                      msg->has_header() ? &msg->header() : nullptr,
                      msg->has_measurement_time(), msg->measurement_time());
  };

  std::shared_ptr<apollo::cyber::ReaderBase> reader;
  if (FLAGS_msg_type == "pointcloud") {
    reader = node->CreateReader<apollo::drivers::PointCloud>(
        FLAGS_channel,
        [&](const std::shared_ptr<apollo::drivers::PointCloud>& msg) {
          record_typed_msg(msg);
        });
  } else if (FLAGS_msg_type == "image") {
    reader = node->CreateReader<apollo::drivers::Image>(
        FLAGS_channel,
        [&](const std::shared_ptr<apollo::drivers::Image>& msg) {
          record_typed_msg(msg);
        });
  } else if (FLAGS_msg_type == "compressed_image") {
    reader = node->CreateReader<apollo::drivers::CompressedImage>(
        FLAGS_channel,
        [&](const std::shared_ptr<apollo::drivers::CompressedImage>& msg) {
          record_typed_msg(msg);
        });
  } else if (FLAGS_msg_type == "imu") {
    reader = node->CreateReader<apollo::drivers::gnss::Imu>(
        FLAGS_channel,
        [&](const std::shared_ptr<apollo::drivers::gnss::Imu>& msg) {
          record_typed_msg(msg);
        });
  } else {
    reader = node->CreateReader<apollo::cyber::message::RawMessage>(
        FLAGS_channel,
        [&](const std::shared_ptr<apollo::cyber::message::RawMessage>& msg) {
          const uint64_t arrival_mono_ns = MonoNowNs();
          const uint64_t recv_unix_ns = RealtimeNowNs();
          apollo::drivers::benchmark::HeaderEnvelope env;
          const bool ok = env.ParsePartialFromArray(
              msg->message.data(), static_cast<int>(msg->message.size()));
          record_header_msg(
              arrival_mono_ns, recv_unix_ns, msg->message.size(),
              (ok && env.has_header()) ? &env.header() : nullptr, false, 0.0);
        });
  }

  if (!reader) {
    std::cerr << "Failed to create reader on channel " << FLAGS_channel << "\n";
    apollo::cyber::Clear();
    return 1;
  }
  if (!FLAGS_ready_file.empty()) {
    std::ofstream ready(FLAGS_ready_file);
    if (!ready.is_open()) {
      std::cerr << "Failed to signal reader readiness\n";
      reader->Shutdown();
      apollo::cyber::Clear();
      return 1;
    }
    ready << "ready\n";
  }

  const uint64_t total_ns =
      warmup_ns +
      static_cast<uint64_t>(std::max(0.0, FLAGS_duration_sec) * 1e9);
  while (!g_stop_requested.load() && !apollo::cyber::IsShutdown()) {
    const uint64_t now_ns = MonoNowNs();
    const uint64_t end = window
        ? __atomic_load_n(&window[1], __ATOMIC_RELAXED) : 0;
    if ((window && end != 0 && now_ns >= end) ||
        (!window && FLAGS_duration_sec > 0.0 &&
         now_ns - start_mono_ns >= total_ns)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  reader->Shutdown();
  const uint64_t end_mono_ns = MonoNowNs();
  const uint64_t actual_start = window
      ? __atomic_load_n(&window[0], __ATOMIC_ACQUIRE) : measure_start_ns;
  const uint64_t actual_end = window
      ? std::min(end_mono_ns, __atomic_load_n(&window[1], __ATOMIC_RELAXED))
      : end_mono_ns;
  const double measured_sec =
      actual_start != 0 && actual_end > actual_start
          ? static_cast<double>(actual_end - actual_start) / 1e9
          : 0.0;
  const auto summary = acc.Finalize(measured_sec, FLAGS_channel, FLAGS_msg_type,
                                    TransportModeName());
  const std::string json =
      apollo::drivers::benchmark::StatsAccumulator::ToJson(summary);

  if (!FLAGS_output_json.empty()) {
    std::ofstream out(FLAGS_output_json, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
      std::cerr << "Failed to open output JSON path: " << FLAGS_output_json
                << "\n";
      apollo::cyber::Clear();
      return 1;
    }
    out << json;
  } else {
    std::cout << json;
  }

  if (window) munmap(const_cast<uint64_t*>(window), 2 * sizeof(uint64_t));
  apollo::cyber::Clear();
  return 0;
}
