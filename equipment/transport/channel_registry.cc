// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/transport/channel_registry.h"

#include <linux/can/error.h>

#include <cerrno>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "modules/drivers/hal/stream/can_stream.h"
#include "modules/drivers/hal/stream/stream_factory.h"

namespace apollo::drivers::equipment {
ChannelRegistry::ChannelRegistry(StreamCreator creator)
    : creator_(std::move(creator)) {}
msg::EquipmentError ChannelRegistry::Initialize(
    const EquipmentModuleConfig& config) {
  std::map<std::string, std::shared_ptr<Channel>> channels;
  std::set<std::string> endpoints;
  std::set<const hal::Stream*> streams;
  for (const auto& entry : config.channels()) {
    auto error = ValidateStream(entry.stream());
    if (!IsOk(error)) {
      return error;
    }
    if (!creator_ || entry.channel_id().empty() ||
        entry.poll_interval_ms() == 0 || entry.receive_queue_capacity() == 0 ||
        entry.receive_queue_capacity() > 65536 ||
        entry.poll_interval_ms() > config.shutdown_timeout_ms() ||
        entry.stream().timeout_usec() >
            static_cast<uint64_t>(config.shutdown_timeout_ms()) * 1000 ||
        (entry.stream().has_can() && !entry.transaction_protocol().empty())) {
      return Error(msg::INVALID_ARGUMENT, "invalid channel identity/timing");
    }
    const auto& stream = entry.stream();
    std::string endpoint;
    if (stream.has_can()) {
      endpoint = "can:" + stream.can().interface_name();
    } else if (stream.has_serial()) {
      std::unique_ptr<char, decltype(&std::free)> resolved(
          ::realpath(stream.serial().device().c_str(), nullptr), &std::free);
      if (!resolved && errno != ENOENT && errno != ENOTDIR) {
        return Error(msg::INVALID_ARGUMENT, "cannot resolve serial endpoint");
      }
      endpoint = "serial:" + (resolved ? std::string(resolved.get())
                                       : stream.serial().device());
    } else {
      const auto& network = stream.has_tcp() ? stream.tcp() : stream.udp();
      endpoint = std::string(stream.has_tcp() ? "tcp:" : "udp:") +
                 network.address() + ":" + std::to_string(network.port());
    }
    if (!endpoints.insert(endpoint).second ||
        channels.count(entry.channel_id())) {
      return Error(msg::INVALID_ARGUMENT, "duplicate channel or endpoint");
    }
    auto transport = creator_(stream);
    if (!transport || !streams.insert(transport.get()).second) {
      return Error(msg::INVALID_ARGUMENT, "missing or shared HAL Stream");
    }
    channels.emplace(entry.channel_id(),
                     std::make_shared<Channel>(entry, std::move(transport)));
  }
  channels_ = std::move(channels);
  return Error(msg::OK);
}
msg::EquipmentError ChannelRegistry::Resolve(const DeviceConfig& config,
                                             ChannelBindings* bindings) const {
  ChannelBindings result;
  for (const auto& entry : config.bindings()) {
    const auto found = channels_.find(entry.channel_id());
    if (entry.name().empty() || found == channels_.end() ||
        !result
             .emplace(entry.name(),
                      ChannelBinding{found->second, entry.address()})
             .second) {
      return Error(msg::INVALID_ARGUMENT, "invalid named channel binding");
    }
  }
  *bindings = std::move(result);
  return Error(msg::OK);
}
msg::EquipmentError ChannelRegistry::Start() {
  for (const auto& entry : channels_) {
    const auto error = entry.second->Start();
    if (!IsOk(error)) {
      return error;
    }
  }
  return Error(msg::OK);
}
msg::EquipmentError ChannelRegistry::Shutdown() {
  auto result = Error(msg::OK);
  for (const auto& entry : channels_) {
    const auto error = entry.second->Shutdown();
    if (!IsOk(error)) {
      result = error;
    }
  }
  return result;
}
std::vector<msg::ChannelHealth> ChannelRegistry::Health() const {
  std::vector<msg::ChannelHealth> result;
  for (const auto& entry : channels_) {
    result.push_back(entry.second->Health());
  }
  return result;
}
msg::EquipmentError ChannelRegistry::ValidateStream(
    const StreamConfig& config) {
  if (!config.has_timeout_usec() || config.timeout_usec() == 0) {
    return Error(msg::INVALID_ARGUMENT, "positive I/O timeout required");
  }
  if ((config.has_can() && !config.can().interface_name().empty()) ||
      (config.has_serial() && !config.serial().device().empty() &&
       config.serial().device().front() == '/' &&
       config.serial().baud_rate() > 0)) {
    return Error(msg::OK);
  }
  if (config.has_tcp() || config.has_udp()) {
    const auto& endpoint = config.has_tcp() ? config.tcp() : config.udp();
    if (!endpoint.address().empty() && endpoint.port() > 0 &&
        endpoint.port() <= 65535) {
      return Error(msg::OK);
    }
  }
  return Error(msg::INVALID_ARGUMENT, "invalid Stream endpoint");
}
std::shared_ptr<hal::Stream> ChannelRegistry::CreateStream(
    const StreamConfig& config) {
  if (config.has_can()) {
    return std::make_shared<hal::CanStream>(
        config.can().interface_name(), config.timeout_usec(),
        config.can().enable_can_fd(), CAN_ERR_BUSOFF);
  }
  if (config.has_serial()) {
    return hal::StreamFactory::CreateSerial(
        config.serial().device(), config.serial().baud_rate(),
        hal::SerialByteSize::B8, hal::SerialParity::None,
        hal::SerialStopBits::One, hal::SerialFlowControl::None,
        config.timeout_usec());
  }
  if (config.has_tcp()) {
    return std::make_shared<hal::TcpStream>(config.tcp().address(),
                                            config.tcp().port(),
                                            config.timeout_usec(), false);
  }
  if (config.has_udp()) {
    return hal::StreamFactory::CreateUdp(
        config.udp().address(), config.udp().port(), config.timeout_usec());
  }
  return nullptr;
}
}  // namespace apollo::drivers::equipment
