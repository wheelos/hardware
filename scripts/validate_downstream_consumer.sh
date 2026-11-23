#!/usr/bin/env bash
set -euo pipefail

# Validate isolated downstream consumption of hardware.* modules without
# pulling unrelated hardware modules or wheelos_transform.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
HARDWARE_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
WORKSPACE_ROOT="${WHEELOS_WORKSPACE_ROOT:-$(cd -- "${HARDWARE_ROOT}/../.." && pwd -P)}"
BAZEL_BIN="${BAZEL_BIN:-$(command -v bazelisk || command -v bazel)}"

if [[ -z "${BAZEL_BIN}" ]]; then
  echo "bazelisk or bazel not found on PATH" >&2
  exit 1
fi

TMP_DIR="$(mktemp -d -t hardware-sdk-consumer-XXXXXX)"
cleanup() {
  chmod -R u+w "${TMP_DIR}" 2>/dev/null || true
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

cp "${HARDWARE_ROOT}/camera/.bazelversion" "${TMP_DIR}/.bazelversion"

cat > "${TMP_DIR}/MODULE.bazel" <<'EOF'
module(name = "hardware_sdk_downstream_smoke", version = "0.1.0")

bazel_dep(name = "wheelos_core", version = "1.0.6", repo_name = "core")
bazel_dep(name = "hardware.camera", version = "0.1.0")
bazel_dep(name = "hardware.equipment", version = "0.1.0")
EOF

cat > "${TMP_DIR}/BUILD.bazel" <<'EOF'
load("@core//runtime:product.bzl", "runtime_product")

runtime_product(
    name = "equipment_smoke",
    default_dag = "dag/equipment/equipment.dag",
    modules = ["@hardware.equipment//:module"],
)
EOF

OVERRIDES=(
  "--lockfile_mode=off"
  "--registry=file://${WORKSPACE_ROOT}/pkgs/bazel-central-registry"
  "--registry=https://bcr.wheelos.cn/"
  "--registry=https://bcr.bazel.build/"
  "--repository_cache=/apollo/.cache/bazel/repo_cache"
  "--disk_cache=/apollo/.cache/bazel/disk_cache"
  "--override_module=wheelos_core=${WORKSPACE_ROOT}/pkgs/core"
  "--override_module=wheelos_common=${WORKSPACE_ROOT}/pkgs/common"
  "--override_module=wheelos_msgs=${WORKSPACE_ROOT}/pkgs/wheelos_msgs"
  "--override_module=hardware.build=${HARDWARE_ROOT}/build"
  "--override_module=hardware.hal=${HARDWARE_ROOT}/hal"
  "--override_module=hardware.camera=${HARDWARE_ROOT}/camera"
  "--override_module=hardware.equipment=${HARDWARE_ROOT}/equipment"
  "--repo_env=APOLLO_SYSROOT_DIR=/opt/apollo/sysroot"
)

STARTUP=(
  "--batch"
  "--ignore_all_rc_files"
  "--output_base=${TMP_DIR}/ob"
)

pushd "${TMP_DIR}" >/dev/null

GRAPH="$("${BAZEL_BIN}" "${STARTUP[@]}" mod graph "${OVERRIDES[@]}")"
echo "${GRAPH}" | grep -q "hardware.camera@"
echo "${GRAPH}" | grep -q "hardware.equipment@"
echo "${GRAPH}" | grep -q "hardware.hal@"
echo "${GRAPH}" | grep -q "hardware.build@"

for forbidden in \
  "wheelos_transform@" \
  "hardware.lidar@" \
  "hardware.radar@" \
  "hardware.vehicle@" \
  "hardware.gnss@" \
  "hardware.camera_gst@" \
  "hesai-lidar-sdk@" \
  "livox-sdk@"
do
  if echo "${GRAPH}" | grep -q "${forbidden}"; then
    echo "Unexpected transitive dependency in isolated consumer: ${forbidden}" >&2
    exit 1
  fi
done

"${BAZEL_BIN}" "${STARTUP[@]}" build "${OVERRIDES[@]}" \
  --cxxopt=-std=c++17 \
  @hardware.camera//:component \
  @hardware.camera//:module \
  @hardware.equipment//:client \
  @hardware.equipment//:component \
  @hardware.equipment//:module \
  //:equipment_smoke

popd >/dev/null
echo "OK: isolated downstream SDK consumer graph, components, and runtime_product verified."
