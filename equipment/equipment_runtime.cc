// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_runtime.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <exception>
#include <fstream>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "cyber/common/log.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "modules/drivers/equipment/operation_store.h"

namespace apollo::drivers::equipment {
namespace {
bool ValidKey(const msg::RequestKey& key) {
  return !key.caller_id().empty() && !key.request_id().empty();
}
std::string TypeName(const google::protobuf::Any& value) {
  const auto slash = value.type_url().rfind('/');
  return slash == std::string::npos ? "" : value.type_url().substr(slash + 1);
}
bool ValidPayload(const google::protobuf::Any& value,
                  const std::string& expected) {
  if (TypeName(value) != expected) {
    return false;
  }
  const auto* descriptor =
      google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
          expected);
  const auto* prototype =
      descriptor
          ? google::protobuf::MessageFactory::generated_factory()->GetPrototype(
                descriptor)
          : nullptr;
  if (!prototype) {
    return false;
  }
  std::unique_ptr<google::protobuf::Message> parsed(prototype->New());
  return parsed->ParseFromString(value.value()) && parsed->IsInitialized();
}
}  // namespace
class EquipmentRuntime::Impl {
 public:
  struct Instance {
    DeviceConfig config;
    msg::DeviceDescriptor descriptor;
    ChannelBindings bindings;
    std::unique_ptr<Device> device;
    std::map<std::string, StateSample> samples;
    std::set<std::string> active;
    std::deque<std::string> queue;
    msg::LinkState link = msg::DISCONNECTED;
    msg::EquipmentError error = Error(msg::NOT_READY, "not started");
    msg::EquipmentError shutdown_error = Error(msg::OK);
    std::condition_variable wake;
    std::thread worker;
  };
  Impl(DeviceFactory factory, ChannelRegistry::StreamCreator creator)
      : factory(std::move(factory)), channels(std::move(creator)) {}
  const msg::CapabilityDescriptor* Capability(const Instance& instance,
                                              const std::string& id) const {
    for (const auto& cap : instance.descriptor.capabilities()) {
      if (cap.capability_id() == id) {
        return &cap;
      }
    }
    return nullptr;
  }
  const msg::ActionDescriptor* Action(const Instance& instance,
                                      const msg::DeviceCommand& command) const {
    const auto* cap = Capability(instance, command.capability_id());
    if (cap) {
      for (const auto& action : cap->actions()) {
        if (action.name() == command.action()) {
          return &action;
        }
      }
    }
    return nullptr;
  }
  msg::DeviceState State(const Instance& instance, uint32_t max_age,
                         const std::string& capability_id = "") const {
    msg::DeviceState state;
    state.set_device_id(instance.config.device_id());
    state.set_link(instance.link);
    auto error = instance.link != msg::CONNECTED
                     ? Error(msg::DEVICE_OFFLINE, instance.error.message())
                 : !running ? Error(msg::NOT_READY, "runtime is stopped")
                            : Error(msg::OK);
    bool ready = IsOk(error);
    for (const auto& id : instance.active) {
      state.add_active_operation_ids(id);
    }
    const auto now = std::chrono::steady_clock::now();
    for (const auto& cap : instance.descriptor.capabilities()) {
      if (!capability_id.empty() && cap.capability_id() != capability_id) {
        continue;
      }
      auto* value = state.add_capabilities();
      const auto found = instance.samples.find(cap.capability_id());
      if (found == instance.samples.end()) {
        value->set_capability_id(cap.capability_id());
        value->set_valid(false);
        value->set_stale(true);
        *value->mutable_error() =
            Error(msg::STATE_UNAVAILABLE, "no device feedback");
      } else {
        *value = found->second.state;
        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - found->second.received_at)
                             .count();
        value->set_age_ms(age);
        value->set_stale(age >
                         std::min(max_age, instance.config.state_max_age_ms()));
        if (value->stale() && IsOk(value->error())) {
          *value->mutable_error() = Error(msg::STALE_STATE, "feedback expired");
        }
      }
      if (!value->valid() || value->stale() || !IsOk(value->error())) {
        ready = false;
        if (IsOk(error)) {
          error = IsOk(value->error())
                      ? Error(msg::STATE_UNAVAILABLE, "invalid state")
                      : value->error();
        }
      }
    }
    state.set_ready(ready);
    state.set_blocked(!instance.active.empty() ||
                      instance.device->HasControlLease() || !ready);
    *state.mutable_error() = error;
    return state;
  }
  void Expire(Instance* instance) {
    for (auto it = instance->active.begin(); it != instance->active.end();) {
      auto* record = operations.Find(*it);
      if (!IsTerminal(record->snapshot.state()) &&
          std::chrono::steady_clock::now() >= record->deadline) {
        operations.Finish(record, msg::TIMED_OUT,
                          Error(msg::DEADLINE_EXCEEDED,
                                "execution deadline exceeded", record->begun));
      }
      if (IsTerminal(record->snapshot.state()) &&
          (!record->begun || record->snapshot.state() == msg::SUCCEEDED ||
           record->snapshot.state() == msg::CANCELED ||
           instance->device->SafeToRelease())) {
        instance->device->ForgetOperation(*it);
        it = instance->active.erase(it);
      } else {
        ++it;
      }
    }
  }
  msg::EquipmentError Apply(Instance* instance, const DeviceUpdate& update) {
    const auto now = std::chrono::steady_clock::now();
    for (const auto& sample : update.samples) {
      const auto& state = sample.state;
      const auto* cap = Capability(*instance, state.capability_id());
      const auto old = instance->samples.find(state.capability_id());
      if (!cap || state.state_sequence() == 0 ||
          state.observed_at_unix_ns() == 0 || !state.has_valid() ||
          !state.error().has_code() ||
          (!state.valid() && IsOk(state.error())) ||
          !ValidPayload(state.value(), cap->state_type()) ||
          sample.received_at == SteadyTime() || sample.received_at > now ||
          (old != instance->samples.end() &&
           (state.state_sequence() <= old->second.state.state_sequence() ||
            sample.received_at < old->second.received_at))) {
        return Error(msg::DEVICE_FAULT, "invalid typed device state", true);
      }
      instance->samples[state.capability_id()] = sample;
    }
    for (const auto& event : update.operations) {
      auto* record = operations.Find(event.operation_id);
      if (!record ||
          record->snapshot.device_id() != instance->config.device_id()) {
        return Error(msg::DEVICE_FAULT, "device reported unknown operation",
                     true);
      }
      if (IsTerminal(record->snapshot.state())) {
        continue;
      }
      const auto* action = Action(*instance, record->snapshot.command());
      if (!event.error.has_code() ||
          (event.state != msg::SUCCEEDED && event.state != msg::FAILED &&
           event.state != msg::CANCELED) ||
          (event.state == msg::FAILED) == IsOk(event.error)) {
        return Error(msg::DEVICE_FAULT, "invalid device operation event", true);
      }
      if (event.state == msg::SUCCEEDED || event.state == msg::CANCELED) {
        const auto& evidence = event.evidence;
        const auto& state = evidence.state;
        if (!record->begun || !state.valid() || state.stale() ||
            !IsOk(state.error()) || state.state_sequence() == 0 ||
            state.capability_id() !=
                record->snapshot.command().capability_id() ||
            !Capability(*instance, state.capability_id()) ||
            !ValidPayload(
                state.value(),
                Capability(*instance, state.capability_id())->state_type()) ||
            evidence.received_at == SteadyTime() ||
            evidence.received_at > now ||
            (!event.already_satisfied &&
             evidence.received_at < record->evidence_after) ||
            now - evidence.received_at >
                std::chrono::milliseconds(
                    instance->config.state_max_age_ms()) ||
            !ValidPayload(event.result, action->result_type())) {
          return Error(msg::DEVICE_FAULT,
                       "completion lacks valid typed evidence", true);
        }
        if (event.state == msg::SUCCEEDED &&
            evidence.received_at > record->deadline) {
          continue;
        }
        *record->snapshot.mutable_completion_state() = state;
        *record->snapshot.mutable_result() = event.result;
        record->snapshot.set_already_satisfied(event.already_satisfied);
      }
      auto error = event.error;
      if (event.state == msg::FAILED) {
        error.set_may_have_executed(record->begun || error.may_have_executed());
      }
      operations.Finish(record, event.state, error);
    }
    Expire(instance);
    return Error(msg::OK);
  }
  void Fault(Instance* instance, const msg::EquipmentError& error) {
    AERROR << instance->config.device_id() << ": " << error.ShortDebugString();
    instance->error = error;
    instance->link = msg::LINK_ERROR;
    for (auto& sample : instance->samples) {
      sample.second.state.set_valid(false);
      *sample.second.state.mutable_error() = error;
    }
    for (const auto& id : instance->active) {
      auto* record = operations.Find(id);
      operations.Finish(record, msg::UNKNOWN,
                        Error(error.code(), error.message(), record->begun));
    }
  }
  void Loop(Instance* instance) {
    while (true) {
      std::string id;
      bool cancel = false;
      msg::DeviceCommand command;
      OperationContext context;
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) {
          return;
        }
        Expire(instance);
        for (const auto& active : instance->active) {
          auto* record = operations.Find(active);
          const auto* action = Action(*instance, record->snapshot.command());
          if (record->begun && !record->stop_attempted &&
              action->supports_cancel() &&
              (record->cancel_requested ||
               IsTerminal(record->snapshot.state()))) {
            record->stop_attempted = true;
            record->evidence_after = std::chrono::steady_clock::now();
            id = active;
            cancel = true;
            break;
          }
        }
        if (id.empty()) {
          while (!instance->queue.empty()) {
            id = instance->queue.front();
            instance->queue.pop_front();
            auto* record = operations.Find(id);
            if (!record || IsTerminal(record->snapshot.state())) {
              id.clear();
              continue;
            }
            if (record->cancel_requested) {
              operations.Finish(record, msg::CANCELED, Error(msg::OK));
              instance->device->ForgetOperation(id);
              instance->active.erase(id);
              id.clear();
              continue;
            }
            record->begun = true;
            record->snapshot.set_state(msg::EXECUTING);
            record->snapshot.set_updated_at_unix_ns(UnixTimeNs());
            record->evidence_after = std::chrono::steady_clock::now();
            command = record->snapshot.command();
            context = {id, record->deadline, record->evidence_after};
            break;
          }
        }
      }
      if (!id.empty()) {
        const auto error = cancel ? instance->device->Cancel(id)
                                  : instance->device->Begin(context, command);
        std::lock_guard<std::mutex> lock(mutex);
        auto* record = operations.Find(id);
        if (IsOk(error)) {
          record->stop_sent = cancel;
        } else {
          AERROR << id << ": " << error.ShortDebugString();
          if (!cancel) {
            operations.Finish(
                record, error.may_have_executed() ? msg::UNKNOWN : msg::FAILED,
                error);
            if (!error.may_have_executed()) {
              instance->device->ForgetOperation(id);
              instance->active.erase(id);
            }
          } else if (!IsTerminal(record->snapshot.state())) {
            record->snapshot.set_state(msg::EXECUTING);
            *record->snapshot.mutable_error() = error;
            record->cancel_requested = false;
          }
        }
      }
      DeviceUpdate update;
      auto error = Error(msg::OK);
      for (const auto& binding : instance->bindings) {
        const auto health = binding.second.channel->Health();
        if (!IsOk(health.error())) {
          error = health.error();
          break;
        }
      }
      if (IsOk(error)) {
        error = instance->device->Poll(&update);
      }
      std::unique_lock<std::mutex> lock(mutex);
      if (IsOk(error)) {
        error = Apply(instance, update);
      }
      if (!IsOk(error)) {
        Fault(instance, error);
        return;
      }
      instance->wake.wait_for(
          lock, std::chrono::milliseconds(instance->config.poll_interval_ms()),
          [this, instance] {
            if (!running || !instance->queue.empty()) {
              return true;
            }
            for (const auto& id : instance->active) {
              const auto* record = operations.Find(id);
              if (record->cancel_requested && !record->stop_attempted) {
                return true;
              }
            }
            return false;
          });
    }
  }
  void Close(Instance* instance) {
    bool owns = instance->device->HasControlLease();
    bool stop_attempted = false;
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (const auto& id : instance->active) {
        auto* record = operations.Find(id);
        owns = owns || record->begun;
        stop_attempted =
            stop_attempted || record->stop_attempted ||
            (record->begun &&
             Action(*instance, record->snapshot.command())->is_stop());
        operations.Finish(
            record, msg::UNKNOWN,
            Error(msg::NOT_READY, "runtime stopped", record->begun));
      }
    }
    try {
      if (owns && !instance->device->SafeToRelease()) {
        const auto error =
            stop_attempted ? Error(msg::OK) : instance->device->RequestStop();
        if (!IsOk(error)) {
          instance->shutdown_error = error;
        } else {
          const auto deadline =
              std::chrono::steady_clock::now() +
              std::chrono::milliseconds(config.shutdown_timeout_ms());
          while (std::chrono::steady_clock::now() < deadline &&
                 !instance->device->SafeToRelease()) {
            DeviceUpdate update;
            auto poll_error = instance->device->Poll(&update);
            {
              std::lock_guard<std::mutex> lock(mutex);
              if (IsOk(poll_error)) {
                poll_error = Apply(instance, update);
              }
              if (!IsOk(poll_error)) {
                instance->shutdown_error = poll_error;
                break;
              }
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(instance->config.poll_interval_ms()));
          }
        }
      }
      if (owns && !instance->device->SafeToRelease()) {
        instance->shutdown_error = Error(
            msg::NOT_READY, "shutdown could not confirm device safe", true);
      }
    } catch (const std::exception& exception) {
      instance->shutdown_error =
          Error(msg::TRANSPORT_ERROR, exception.what(), true);
    }
    try {
      const auto error = instance->device->Deactivate();
      if (IsOk(instance->shutdown_error) && !IsOk(error)) {
        instance->shutdown_error = error;
      }
      std::lock_guard<std::mutex> lock(mutex);
      instance->link = IsOk(error) ? msg::DISCONNECTED : msg::LINK_ERROR;
    } catch (const std::exception& exception) {
      instance->shutdown_error =
          Error(msg::TRANSPORT_ERROR, exception.what(), true);
      std::lock_guard<std::mutex> lock(mutex);
      instance->link = msg::LINK_ERROR;
    }
    if (!IsOk(instance->shutdown_error)) {
      AERROR << instance->config.device_id() << ": "
             << instance->shutdown_error.ShortDebugString();
    }
  }
  void Run(Instance* instance, std::promise<msg::EquipmentError> startup) {
    bool announced = false;
    try {
      const auto error = instance->device->Activate();
      {
        std::lock_guard<std::mutex> lock(mutex);
        instance->link = IsOk(error) ? msg::CONNECTED : msg::LINK_ERROR;
        instance->error = error;
      }
      startup.set_value(error);
      announced = true;
      if (IsOk(error)) {
        Loop(instance);
      }
    } catch (const std::exception& exception) {
      const auto error = Error(msg::TRANSPORT_ERROR, exception.what(), true);
      {
        std::lock_guard<std::mutex> lock(mutex);
        Fault(instance, error);
      }
      if (!announced) {
        startup.set_value(error);
      }
    }
    Close(instance);
  }
  mutable std::mutex mutex;
  std::mutex lifecycle;
  DeviceFactory factory;
  ChannelRegistry channels;
  EquipmentModuleConfig config;
  std::map<std::string, std::unique_ptr<Instance>> instances;
  OperationStore operations;
  std::string instance_id;
  uint64_t next_id = 0;
  bool initialized = false;
  bool started = false;
  bool running = false;
};

EquipmentRuntime::EquipmentRuntime(DeviceFactory factory,
                                   ChannelRegistry::StreamCreator creator)
    : impl_(std::make_unique<Impl>(std::move(factory), std::move(creator))) {}
EquipmentRuntime::~EquipmentRuntime() {
  const auto error = Shutdown();
  if (!IsOk(error)) {
    AERROR << error.ShortDebugString();
  }
}
msg::EquipmentError EquipmentRuntime::Initialize(
    const EquipmentModuleConfig& config) {
  std::lock_guard<std::mutex> lifecycle(impl_->lifecycle);
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (impl_->initialized) {
    return Error(msg::NOT_READY, "runtime already initialized");
  }
  if (config.devices_size() == 0 || config.service_root().empty() ||
      config.service_root().front() != '/' ||
      config.service_root().back() == '/' || config.max_operations() == 0 ||
      config.result_retention_ms() == 0 ||
      config.result_retention_ms() >
          static_cast<uint64_t>(
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  SteadyTime::duration::max())
                  .count()) ||
      config.max_execution_timeout_ms() == 0 ||
      config.shutdown_timeout_ms() == 0) {
    return Error(msg::INVALID_ARGUMENT, "invalid equipment module limits");
  }
  std::string instance_id;
  std::ifstream uuid("/proc/sys/kernel/random/uuid");
  if (!(uuid >> instance_id) || instance_id.empty()) {
    return Error(msg::NOT_READY, "cannot generate runtime instance ID");
  }
  std::map<std::string, std::unique_ptr<Impl::Instance>> instances;
  try {
    auto error = impl_->channels.Initialize(config);
    if (!IsOk(error)) {
      return error;
    }
    for (const auto& entry : config.devices()) {
      if (entry.device_id().empty() || entry.device_type().empty() ||
          entry.model().empty() || entry.state_max_age_ms() == 0 ||
          entry.poll_interval_ms() == 0 ||
          entry.poll_interval_ms() > config.shutdown_timeout_ms()) {
        return Error(msg::INVALID_ARGUMENT, "invalid device identity/timing");
      }
      auto instance = std::make_unique<Impl::Instance>();
      instance->config = entry;
      error = impl_->channels.Resolve(entry, &instance->bindings);
      if (!IsOk(error)) {
        return error;
      }
      instance->device = impl_->factory.Create(entry, instance->bindings);
      if (!instance->device) {
        return Error(msg::INVALID_ARGUMENT,
                     "unknown device/model or incompatible bindings");
      }
      error = instance->device->Initialize(entry);
      if (!IsOk(error)) {
        return error;
      }
      instance->descriptor = instance->device->Describe();
      std::set<std::string> channel_ids;
      for (const auto& binding : entry.bindings()) {
        if (channel_ids.insert(binding.channel_id()).second) {
          instance->descriptor.add_channel_ids(binding.channel_id());
        }
      }
      const auto& descriptor = instance->descriptor;
      if (descriptor.device_id() != entry.device_id() ||
          descriptor.device_type() != entry.device_type() ||
          descriptor.model() != entry.model() ||
          descriptor.capabilities_size() == 0) {
        return Error(msg::INVALID_ARGUMENT, "invalid device descriptor");
      }
      std::set<std::string> caps;
      for (const auto& cap : descriptor.capabilities()) {
        if (cap.capability_id().empty() || cap.type().empty() ||
            cap.state_type().empty() || cap.version() == 0 ||
            !caps.insert(cap.capability_id()).second ||
            cap.actions_size() == 0) {
          return Error(msg::INVALID_ARGUMENT, "invalid capability descriptor");
        }
        std::set<std::string> actions;
        for (const auto& action : cap.actions()) {
          if (action.name().empty() || action.request_type().empty() ||
              action.result_type().empty() ||
              !actions.insert(action.name()).second) {
            return Error(msg::INVALID_ARGUMENT, "invalid action descriptor");
          }
        }
      }
      if (!instances.emplace(entry.device_id(), std::move(instance)).second) {
        return Error(msg::INVALID_ARGUMENT, "duplicate device ID");
      }
    }
  } catch (const std::exception& exception) {
    return Error(msg::INVALID_ARGUMENT, exception.what());
  }
  impl_->config = config;
  impl_->instances = std::move(instances);
  impl_->instance_id = instance_id;
  impl_->initialized = true;
  return Error(msg::OK);
}
msg::EquipmentError EquipmentRuntime::Start() {
  std::lock_guard<std::mutex> lifecycle(impl_->lifecycle);
  std::vector<std::future<msg::EquipmentError>> futures;
  auto error = Error(msg::OK);
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->initialized || impl_->started) {
      return Error(msg::NOT_READY, "initialize a new runtime before starting");
    }
    futures.reserve(impl_->instances.size());
    impl_->started = true;
    error = impl_->channels.Start();
    if (!IsOk(error)) {
      const auto cleanup = impl_->channels.Shutdown();
      if (!IsOk(cleanup)) {
        AERROR << cleanup.ShortDebugString();
      }
      return error;
    }
    impl_->running = true;
    try {
      for (auto& entry : impl_->instances) {
        std::promise<msg::EquipmentError> promise;
        auto future = promise.get_future();
        entry.second->worker = std::thread(
            &Impl::Run, impl_.get(), entry.second.get(), std::move(promise));
        futures.push_back(std::move(future));
      }
    } catch (const std::system_error& exception) {
      error = Error(msg::RESOURCE_EXHAUSTED, exception.what());
      impl_->running = false;
    }
  }
  for (auto& future : futures) {
    const auto result = future.get();
    if (!IsOk(result)) {
      error = result;
    }
  }
  if (!IsOk(error)) {
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      impl_->running = false;
      for (auto& entry : impl_->instances) {
        entry.second->wake.notify_all();
      }
    }
    for (auto& entry : impl_->instances) {
      if (entry.second->worker.joinable()) {
        entry.second->worker.join();
      }
    }
    const auto cleanup = impl_->channels.Shutdown();
    if (!IsOk(cleanup)) {
      AERROR << cleanup.ShortDebugString();
    }
  }
  return error;
}
msg::EquipmentError EquipmentRuntime::Shutdown() {
  std::lock_guard<std::mutex> lifecycle(impl_->lifecycle);
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->running = false;
    for (auto& entry : impl_->instances) {
      entry.second->wake.notify_all();
    }
  }
  auto error = Error(msg::OK);
  for (auto& entry : impl_->instances) {
    if (entry.second->worker.joinable()) {
      entry.second->worker.join();
    }
    if (!IsOk(entry.second->shutdown_error)) {
      error = entry.second->shutdown_error;
    }
  }
  const auto channel_error = impl_->channels.Shutdown();
  if (!IsOk(channel_error)) {
    error = channel_error;
  }
  return error;
}
msg::ListDevicesResponse EquipmentRuntime::ListDevices(
    const msg::ListDevicesRequest& request) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  msg::ListDevicesResponse result;
  result.set_runtime_instance_id(impl_->instance_id);
  result.set_result_retention_ms(impl_->config.result_retention_ms());
  for (const auto& health : impl_->channels.Health()) {
    *result.add_channels() = health;
  }
  *result.mutable_error() = Error(msg::OK);
  if (!impl_->initialized) {
    *result.mutable_error() = Error(msg::NOT_READY, "runtime not initialized");
  } else if (!request.device_id().empty() &&
             impl_->instances.count(request.device_id()) == 0) {
    *result.mutable_error() = Error(msg::NOT_FOUND, "unknown device");
  } else {
    for (const auto& entry : impl_->instances) {
      if (request.device_id().empty() || request.device_id() == entry.first) {
        *result.add_devices() = entry.second->descriptor;
      }
    }
  }
  return result;
}
msg::GetStateResponse EquipmentRuntime::GetState(
    const msg::GetStateRequest& request) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  msg::GetStateResponse result;
  result.set_runtime_instance_id(impl_->instance_id);
  const auto found = impl_->instances.find(request.device_id());
  if (!impl_->initialized) {
    *result.mutable_error() = Error(msg::NOT_READY, "runtime not initialized");
  } else if (request.device_id().empty() || request.max_age_ms() == 0) {
    *result.mutable_error() =
        Error(msg::INVALID_ARGUMENT, "device and max age required");
  } else if (found == impl_->instances.end()) {
    *result.mutable_error() = Error(msg::NOT_FOUND, "unknown device");
  } else if (!request.capability_id().empty() &&
             !impl_->Capability(*found->second, request.capability_id())) {
    *result.mutable_error() =
        Error(msg::UNSUPPORTED_CAPABILITY, "unknown capability");
  } else {
    *result.mutable_state() = impl_->State(*found->second, request.max_age_ms(),
                                           request.capability_id());
    *result.mutable_error() = result.state().error();
  }
  return result;
}
msg::ExecuteResponse EquipmentRuntime::Execute(
    const msg::ExecuteRequest& request) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  msg::ExecuteResponse result;
  auto fail = [&result](msg::ErrorCode code, const std::string& message) {
    *result.mutable_error() = Error(code, message);
    return result;
  };
  if (!impl_->running) {
    return fail(msg::NOT_READY, "runtime not running");
  }
  if (request.runtime_instance_id() != impl_->instance_id) {
    return fail(msg::RUNTIME_CHANGED, "runtime instance does not match");
  }
  if (!ValidKey(request.request_key()) || request.device_id().empty() ||
      request.execution_timeout_ms() == 0 ||
      request.execution_timeout_ms() >
          impl_->config.max_execution_timeout_ms()) {
    return fail(msg::INVALID_ARGUMENT,
                "invalid identity, request key or timeout");
  }
  std::set<std::string> protected_ids;
  for (const auto& instance : impl_->instances) {
    protected_ids.insert(instance.second->active.begin(),
                         instance.second->active.end());
  }
  impl_->operations.Prune(impl_->config.result_retention_ms(), protected_ids);
  if (auto* previous = impl_->operations.Find(request.request_key())) {
    const auto& snapshot = previous->snapshot;
    if (snapshot.device_id() != request.device_id() ||
        snapshot.command().SerializeAsString() !=
            request.command().SerializeAsString() ||
        snapshot.execution_timeout_ms() != request.execution_timeout_ms()) {
      return fail(msg::REQUEST_CONFLICT,
                  "request key already has another command");
    }
    *result.mutable_error() = Error(msg::OK);
    *result.mutable_operation() = snapshot;
    return result;
  }
  const auto found = impl_->instances.find(request.device_id());
  if (found == impl_->instances.end()) {
    return fail(msg::NOT_FOUND, "unknown device");
  }
  auto& instance = *found->second;
  const auto* action = impl_->Action(instance, request.command());
  if (!action) {
    return fail(msg::UNSUPPORTED_CAPABILITY, "unknown capability or action");
  }
  if (!ValidPayload(request.command().arguments(), action->request_type())) {
    return fail(msg::INVALID_ARGUMENT, "action payload type mismatch");
  }
  if (!action->supports_completion_feedback()) {
    return fail(msg::COMPLETION_UNSUPPORTED, "no completion feedback");
  }
  if (instance.link != msg::CONNECTED) {
    return fail(msg::DEVICE_OFFLINE, "device is disconnected");
  }
  for (const auto& binding : instance.bindings) {
    if (!IsOk(binding.second.channel->Health().error())) {
      return fail(msg::DEVICE_OFFLINE, "bound channel is unavailable");
    }
  }
  if (!instance.active.empty() && !action->is_stop()) {
    return fail(msg::BUSY, "device has an outstanding operation");
  }
  if (action->is_stop()) {
    for (const auto& id : instance.active) {
      if (impl_
              ->Action(instance, impl_->operations.Find(id)->snapshot.command())
              ->is_stop()) {
        return fail(msg::BUSY, "stop already pending");
      }
    }
  }
  const auto error = instance.device->Validate(request.command());
  if (!IsOk(error)) {
    return fail(error.code(), error.message());
  }
  const uint64_t capacity =
      static_cast<uint64_t>(impl_->config.max_operations()) +
      (action->is_stop() ? impl_->instances.size() : 0);
  if (impl_->operations.size() >= capacity) {
    return fail(msg::RESOURCE_EXHAUSTED, "operation retention table full");
  }
  const auto id = impl_->instance_id + ":" + std::to_string(++impl_->next_id);
  msg::OperationSnapshot snapshot;
  snapshot.set_runtime_instance_id(impl_->instance_id);
  snapshot.set_operation_id(id);
  *snapshot.mutable_request_key() = request.request_key();
  snapshot.set_device_id(request.device_id());
  *snapshot.mutable_command() = request.command();
  snapshot.set_execution_timeout_ms(request.execution_timeout_ms());
  snapshot.set_accepted_at_unix_ns(UnixTimeNs());
  snapshot.set_updated_at_unix_ns(snapshot.accepted_at_unix_ns());
  snapshot.set_state(msg::ACCEPTED);
  *snapshot.mutable_error() = Error(msg::OK);
  impl_->operations.Insert(
      snapshot, std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(request.execution_timeout_ms()));
  instance.active.insert(id);
  if (action->is_stop()) {
    instance.queue.push_front(id);
  } else {
    instance.queue.push_back(id);
  }
  instance.wake.notify_one();
  *result.mutable_error() = Error(msg::OK);
  *result.mutable_operation() = snapshot;
  return result;
}
msg::GetOperationResponse EquipmentRuntime::GetOperation(
    const msg::GetOperationRequest& request) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  msg::GetOperationResponse result;
  if (!impl_->initialized) {
    *result.mutable_error() = Error(msg::NOT_READY, "runtime not initialized");
  } else if (request.runtime_instance_id() != impl_->instance_id) {
    *result.mutable_error() =
        Error(msg::RUNTIME_CHANGED, "runtime instance does not match");
  } else if ((!request.has_operation_id() || request.operation_id().empty()) &&
             (!request.has_request_key() || !ValidKey(request.request_key()))) {
    *result.mutable_error() =
        Error(msg::INVALID_ARGUMENT, "operation ID or key required");
  } else {
    const auto* record = request.has_request_key()
                             ? impl_->operations.Find(request.request_key())
                             : impl_->operations.Find(request.operation_id());
    if (!record) {
      *result.mutable_error() =
          Error(msg::RESULT_UNAVAILABLE, "unknown or expired operation");
    } else {
      *result.mutable_error() = Error(msg::OK);
      *result.mutable_operation() = record->snapshot;
    }
  }
  return result;
}
msg::CancelOperationResponse EquipmentRuntime::CancelOperation(
    const msg::CancelOperationRequest& request) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  msg::CancelOperationResponse result;
  auto fail = [&result](msg::ErrorCode code, const std::string& message) {
    *result.mutable_error() = Error(code, message);
    return result;
  };
  if (!impl_->initialized) {
    return fail(msg::NOT_READY, "runtime not initialized");
  }
  if (request.runtime_instance_id() != impl_->instance_id) {
    return fail(msg::RUNTIME_CHANGED, "runtime instance does not match");
  }
  if (request.operation_id().empty()) {
    return fail(msg::INVALID_ARGUMENT, "operation ID required");
  }
  auto* record = impl_->operations.Find(request.operation_id());
  if (!record) {
    return fail(msg::RESULT_UNAVAILABLE, "unknown or expired operation");
  }
  auto& instance = *impl_->instances.at(record->snapshot.device_id());
  const auto* action = impl_->Action(instance, record->snapshot.command());
  if (!IsTerminal(record->snapshot.state())) {
    if (record->begun && !action->supports_cancel()) {
      return fail(msg::CANCEL_UNSUPPORTED, "action has no safe cancellation");
    }
    if (record->stop_attempted && !record->stop_sent) {
      return fail(msg::CANCEL_REJECTED, "previous cancellation attempt failed");
    }
    record->cancel_requested = true;
    record->snapshot.set_state(msg::CANCELING);
    record->snapshot.set_updated_at_unix_ns(UnixTimeNs());
    instance.wake.notify_one();
  }
  *result.mutable_error() = Error(msg::OK);
  *result.mutable_operation() = record->snapshot;
  return result;
}
}  // namespace apollo::drivers::equipment
