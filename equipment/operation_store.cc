// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/operation_store.h"

#include <set>
#include <string>

namespace apollo::drivers::equipment {
OperationStore::Record* OperationStore::Find(const std::string& id) {
  const auto found = records_.find(id);
  return found == records_.end() ? nullptr : &found->second;
}
const OperationStore::Record* OperationStore::Find(
    const std::string& id) const {
  const auto found = records_.find(id);
  return found == records_.end() ? nullptr : &found->second;
}
OperationStore::Record* OperationStore::Find(const msg::RequestKey& key) {
  const auto found = keys_.find({key.caller_id(), key.request_id()});
  return found == keys_.end() ? nullptr : Find(found->second);
}
OperationStore::Record& OperationStore::Insert(
    const msg::OperationSnapshot& snapshot, SteadyTime deadline) {
  auto& record = records_[snapshot.operation_id()];
  record.snapshot = snapshot;
  record.deadline = deadline;
  record.evidence_after = std::chrono::steady_clock::now();
  keys_.emplace(Key{snapshot.request_key().caller_id(),
                    snapshot.request_key().request_id()},
                snapshot.operation_id());
  return record;
}
void OperationStore::Finish(Record* record, msg::OperationState state,
                            const msg::EquipmentError& error) {
  if (!IsTerminal(record->snapshot.state())) {
    record->snapshot.set_state(state);
    record->snapshot.set_updated_at_unix_ns(UnixTimeNs());
    *record->snapshot.mutable_error() = error;
    record->finished_at = std::chrono::steady_clock::now();
  }
}
void OperationStore::Prune(uint64_t retention_ms,
                           const std::set<std::string>& protected_ids) {
  const auto now = std::chrono::steady_clock::now();
  for (auto it = records_.begin(); it != records_.end();) {
    const auto& record = it->second;
    if (IsTerminal(record.snapshot.state()) &&
        protected_ids.count(it->first) == 0 &&
        now - record.finished_at > std::chrono::milliseconds(retention_ms)) {
      keys_.erase({record.snapshot.request_key().caller_id(),
                   record.snapshot.request_key().request_id()});
      it = records_.erase(it);
    } else {
      ++it;
    }
  }
}
}  // namespace apollo::drivers::equipment
