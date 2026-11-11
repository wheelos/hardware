# Equipment control

`hardware.equipment` is a device-first control module, independent of
`hardware.vehicle`. Device business logic owns capabilities, interlocks,
completion criteria and continuing sessions. Protocol backends own wire
formats and reuse existing HAL Streams.

```text
EquipmentClient / capability clients
       -> Cyber services -> EquipmentRuntime / OperationStore
       -> Device -> typed Backend -> Channel / protocol session -> HAL Stream
```

## Modules and extension

| Class / target | Responsibility |
| --- | --- |
| `Device`, `:device` | Device lifecycle, typed command contract and reusable envelope validation |
| `SwitchDevice`, `OpenCloseDevice`, `LiftDevice`, `:binary_devices` | Basic capabilities, sharing binary-target business logic |
| `ChargerDevice`, `:charger` | Charging operations and continuing session ownership |
| `BinaryBackend`, `ChargingBackend` | Typed device/backend boundaries, not another byte-stream abstraction |
| `CanSignalBackend`, `:can_signal_backend` | Static CAN payloads and bit feedback |
| `DeviceFactory`, `:factory` | Explicit `(device_type, model)` construction |
| `ChannelRegistry`, `:channel_registry` | Named channels, binding resolution, endpoint uniqueness and lifecycle |
| `Channel`, `CanSession`, `TransactionSession`, `:channel` | One CAN reader, frame routing, bounded subscriber queues and serialized byte-protocol transactions |
| `SimBinaryBackend`, `SimChargingBackend`, `:simulation` | Explicit configuration-selected software models, without physical I/O |
| `EquipmentRuntime`, `:runtime` | Workers, admission, scheduling and cached snapshots |
| `OperationStore`, `:operation_store` | Bounded results, request-key deduplication and retention |
| `EquipmentService`, `:service` | Five Cyber RPC endpoints |
| `EquipmentClient`, `:client` | Generic RPC and typed `Submit` |
| `BinaryClient`, `ChargingClient`, `:client` | Capability-specific convenience APIs |
| `EquipmentComponent`, `:component` | Loadable `libequipment_component.so` |

Headers use `modules/drivers/equipment/`. Business implementations live in
`devices/`; model adapters in `protocols/`; endpoint bookkeeping in `transport/`;
typed SDK facades in `clients/`. Public messages are
`wheelos_msgs/equipment_msgs` (`apollo.equipment`). Local configuration is
`proto/equipment_config.proto`.

All devices derive from `Device`, not from `SwitchDevice`. Add a new capability
schema and Device for new business semantics, or a new backend for another
model of an existing device. Register its factory explicitly: no Runtime
business branch or global command enum needs changing.
`EquipmentComponent::CreateFactory()` can be overridden by a product component
to supply compiled-in model registrations; a standalone application can
construct `EquipmentRuntime(factory)` and register `EquipmentService`.
Unknown models fail initialization, never fall back to a test adapter.

`Activate/Deactivate` mean software/transport lifecycle, not physical business
start/stop. A single worker owns each device's activation, backend I/O,
cancellation and deactivation. Query/admission methods must be thread-safe.
Backends receive named `ChannelBindings`, not a newly created Stream.
They must acquire CAN sessions or exclusive/transactional byte access during
initialization, before channels start. Devices do not own transport claims.
Backends must bound I/O, preserve framing, validate responses, and report
partial writes explicitly rather than automatically retrying physical commands.

## Capabilities and services

The default root is `/equipment`.

| Suffix | Request / response | Meaning |
| --- | --- | --- |
| `/list` | `ListDevicesRequest/Response` | Descriptors, runtime instance ID and result-retention window |
| `/execute` | `ExecuteRequest/Response` | Accept/deduplicate a command and return its operation |
| `/get_state` | `GetStateRequest/Response` | Observed state with an explicit maximum age |
| `/get_operation` | `GetOperationRequest/Response` | Query by operation ID or caller/request key |
| `/cancel` | `CancelOperationRequest/Response` | Cancel a pending operation when supported |

`ListDevicesResponse.channels` reports channel health and CAN frame counters;
each device descriptor identifies its bound channel IDs.
Health is a copied snapshot, not an instruction to retry or resume a device.

`DeviceCommand` identifies a capability and action and carries a typed
`google.protobuf.Any` goal. Descriptors declare request, result and state types.
Mismatched or malformed goals are rejected; there is no arbitrary JSON fallback.
Capability states have independent sequences, validity and feedback ages.
Generic state does not require mechanical motion or position.
`ready/blocked` are query summaries, not universal admission rules.

`BinaryClient` exposes `SetEnabled`, `Open`, `Close`, `Raise`, `Lower`.
`ChargingClient` exposes `StartCharging` and `StopCharging`.
Supply a discovered `DeviceTarget`, the listed runtime instance ID,
`(caller_id, request_id)`, execution timeout and RPC timeout.
Both facades use the generic `EquipmentClient::Submit`; future facades can
add typed operations without editing that generic client.

**Execute acceptance is not physical completion.** Poll `GetOperation`.
Writes, echoes and ACKs are not target evidence. A binary operation succeeds
on valid stationary target feedback; an already-satisfied target requires no
write and returns `already_satisfied=true`.

Charging start succeeds when feedback confirms `CHARGING` and returns a
`ChargingResult.session_id`, not when the battery is full. The Device continues
holding that session after its start operation finishes. A new start is busy;
an explicit stop with the current session ID can enter and completes on `IDLE`.
Canceling a successful start does not stop its continuing session.
Canceling a pending start requires stop confirmation.

The SDK never automatically resends a physical command. On an execute RPC
timeout, `may_have_executed=true`: query with the original request key.
Identical original command content and timeout return the same retained
operation; changed content with that key is `REQUEST_CONFLICT`.
After restart, `RUNTIME_CHANGED` invalidates old assumptions.
Unknown/expired results are `RESULT_UNAVAILABLE`, not proof of no execution.

The result table preserves retry evidence for its retention window and rejects
new ordinary actions at capacity. Bounded extra slots reserve a stop path.
Ordinary actions are serialized per device; explicit stops can interrupt an
unfinished operation. Timeout/unknown outcomes retain possibly active control
until valid safety feedback. Late success never rewrites a terminal timeout.
Shutdown stops only owned operations/sessions and reports inability to confirm
safety. Its polling budget is not a hardware emergency stop or a guarantee
that an unbounded vendor API can be interrupted.

## Protocol and configuration boundaries

The default configuration deliberately has no devices and refuses startup.
There is no guessed physical-device profile.
Define `EquipmentModuleConfig.channels` separately from devices. Each
`ChannelConfig` has a unique ID, HAL Stream endpoint, bounded I/O/poll settings
and receive-queue capacity. Each `DeviceConfig.bindings` entry identifies a
required backend binding name, channel ID and typed protocol address.
The same device can bind several channels; several CAN devices can share one
channel. Backend-required names and address types are validated at startup.
Factories still select a compatible Device/backend pair by `(device_type, model)`.

The old unpublished `DeviceConfig.stream` field is retired and reserved.
Move it to a named channel and add a binding. Move the old CAN mapping IDs to
`DeviceBinding.address` as `CanSignalAddress`; payloads/bits remain in
`CanSignalConfig`. This is a coordinated source/configuration migration,
not compatibility with the earlier unpublished draft.

Built-in registrations are `switch`, `open_close`, `lift` with `can_signal`.
They require exactly one `main` binding with `CanSignalAddress`:
command/feedback IDs, standard/extended addressing and an optional CAN FD
requirement. `CanSignalConfig` holds static positive/negative/optional-stop
payloads, feedback length, distinct LSB0 limit/motion bits and an optional fault.
Both limit bits active or a fault bit yields invalid fault state.
This backend supports classic CAN and HAL's fixed-slot CAN FD interface.

Rolling counters, CRC, authentication, cyclic/watchdog control and charger
handshakes require a dedicated model backend. DBC may generate codecs inside
that backend; DBC generation is not included.
Charger business logic and its backend interface are provided, but no production
charger protocol is shipped. Refill and arm are future extensions, not empty
devices reported as supported. Arm motion remains owned by a dedicated robot
controller rather than a new servo/planning stack here.

ChannelRegistry supports CAN, serial (8N1), TCP and UDP Streams.
It rejects duplicate IDs, duplicate endpoints and shared Stream pointers.
Existing serial symlinks are resolved before ownership checks; network endpoint
checks compare configured address/port, not all DNS or network aliases.
Bitrates/interface setup remain deployment responsibilities, not runtime actions.
Unused configured channels are never connected.

CAN has one receive thread per channel, indexed ID/format routing and bounded
per-session queues. Read-only feedback subscriptions may overlap; duplicate
transmit IDs and transmit/feedback ownership overlap are rejected.
Subscriber overflow fails that subscriber explicitly; channel I/O errors or
bus-off invalidate all bound devices, not devices on other channels.
The equipment CAN creator opts into `CAN_ERR_BUSOFF` through HAL's new
four-argument CanStream constructor. The existing three-argument constructor
retains its default no-error-frame subscription policy for other callers.
Kernel filter configuration is checked on an unbound socket; physical bus-off
behavior still requires device-level validation.
CAN receipt and dispatch timestamps are sampled under the same channel I/O lock.
Backend command/stop methods return that dispatch boundary through `sent_at`;
Devices require feedback strictly newer than it for completion and release.
This rejects frames read before dispatch, even if delivered afterward, without
rejecting fast replies received before the Backend call returns.
These are host I/O ordering guarantees, not device-side causal correlation:
frames still queued in the kernel or on the bus require model-specific response
correlation when receipt ordering alone is insufficient.
HAL does not guarantee thread safety: channel reads and writes share an I/O lock.

Byte channels default to exclusive access through the existing HAL Stream,
without duplicating its read/write interface. To share serial/TCP, configure
`transaction_protocol` and acquire sessions with distinct protocol addresses.
The backend must canonicalize those protocol-specific address keys.
`TransactionSession::Run` holds the channel I/O lock only for one complete
bounded protocol transaction, not the entire mechanical/charging operation.
The model backend owns framing, response-address/CRC checks, late-response
recovery and frame gaps; no Modbus implementation is implied. Callbacks must
not nest channel transactions or wait on device/service work.
Backend activation/deactivation never connects/disconnects the shared transport;
Runtime starts channels before devices and closes them after device shutdown.
Exclusive byte backends report transport failures through their Channel.
TCP automatic reconnect is disabled; channel faults are sticky for this
runtime. Recovery must re-synchronize observed state, never replay commands.
SocketCAN interfaces may be shared by independent sockets, but deployment must
coordinate transmit IDs, ECU ownership, interface setup and bus load with
vehicle and other processes. In-process checks do not prove cross-process
exclusivity. Likewise, service discovery is not a distributed startup lock.

## Configuration-selected simulation

`conf/simulation.pb.txt` defines explicit `sim` models for switch, lift and
charger with no channels or hardware bindings. The factory also supports
`open_close/sim`. Select the model and remove hardware bindings in configuration;
no source change or HAL installation is needed.
Optional `SimulationConfig` sets the initial binary position, transition delay,
fault-on-command and frozen feedback. Simulated feedback exercises the same
Device state machines and Operation contract; it is not manufacturer evidence.
Clocks remain real monotonic clocks; accelerated/virtual time is not provided.

Communication health, typed business state and Operation state remain separate.
No universal PackML/EStop state machine is imposed. State queries copy protected
data; moving writers onto one executor does not permit unsynchronized queries.
The five request/response services remain authoritative; event streaming,
caller-control leases and redundant transports are not implemented.

## Build and evidence

This is an unreleased module and additive, unreleased message interface.
Use coordinated local source overrides; the published wheelos_msgs version
alone does not establish that the new targets exist. Registry publication
and root-product promotion are separate steps.
The optional HAL error-filter overload is also a coordinated source change;
this is not a binary-compatibility claim for prebuilt HAL consumers.

Run in the mapped-user managed container with Bazel 8.8.1, the product's
registries/shared caches, and local overrides for `wheelos_core`,
`wheelos_msgs`, `wheelos_common`, `hardware.hal`, `hardware.build`:

```text
build //:component //:client //:charger
test //tests:equipment_runtime_test //tests:charger_device_test //tests:equipment_service_test //tests:equipment_stream_test //tests:channel_test //tests:simulation_test
test @hardware.hal//stream:can_stream_regression_test
```

Tests cover shared CAN routing, broadcast subscribers, ownership, queue overflow,
multi-binding, bus-off isolation, transaction serialization and explicit sim
models, alongside the operation/session regressions, real loopback
HAL TCP and real loopback Cyber RPC. No physical device or network interface
is configured. These checks do not establish vendor behavior, mechanical
limits, electrical timing, bus-load safety or functional-safety certification.
Packaging uses `:package_files` / `:install_dir`; Core's runtime assembly
collects transitive runtime libraries.
