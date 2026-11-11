// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include "modules/drivers/equipment/device.h"

namespace apollo::drivers::equipment {
// Synchronization belongs to EquipmentRuntime.
class OperationStore {
 public:
  struct Record {
    msg::OperationSnapshot snapshot;
    SteadyTime deadline;
    SteadyTime finished_at;
    SteadyTime evidence_after;
    bool begun = false;
    bool cancel_requested = false;
    bool stop_attempted = false;
    bool stop_sent = false;
  };
  Record* Find(const std::string& id);
  const Record* Find(const std::string& id) const;
  Record* Find(const msg::RequestKey& key);
  Record& Insert(const msg::OperationSnapshot& snapshot, SteadyTime deadline);
  void Finish(Record* record, msg::OperationState state,
              const msg::EquipmentError& error);
  void Prune(uint64_t retention_ms, const std::set<std::string>& protected_ids);
  size_t size() const { return records_.size(); }

 private:
  using Key = std::pair<std::string, std::string>;
  std::map<std::string, Record> records_;
  std::map<Key, std::string> keys_;
};
}  // namespace apollo::drivers::equipment
