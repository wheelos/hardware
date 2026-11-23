#!/usr/bin/env python3
"""Validate multi-module Bzlmod structure and version alignment in wheelos/hardware."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

MODULES = (
    "audio",
    "build",
    "camera",
    "camera_gst",
    "canbus",
    "equipment",
    "gnss",
    "hal",
    "image_decompress",
    "lidar",
    "radar",
    "vehicle",
    "video",
)

EXPECTED_VERSIONS = {
    "abseil-cpp": "20260107.1",
    "bazel_skylib": "1.9.2",
    "gflags": "2.3.1",
    "googletest": "1.17.0.bcr.2",
    "platforms": "1.1.0",
    "protobuf": "36.2",
    "rules_cc": "0.2.26",
    "rules_pkg": "1.3.0",
    "wheelos_common": "0.1.3",
    "wheelos_core": "1.0.6",
    "wheelos_msgs": "0.1.6",
    "wheelos_transform": "0.1.0",
    "yaml-cpp": "0.9.0.bcr.1",
}

RUNTIME_MODULE_REQUIRED = (
    "audio",
    "camera",
    "camera_gst",
    "canbus",
    "equipment",
    "gnss",
    "image_decompress",
    "lidar",
    "radar",
    "vehicle",
    "video",
)

COMPONENT_ALIAS_REQUIRED = (
    "audio",
    "camera",
    "camera_gst",
    "equipment",
    "gnss",
    "image_decompress",
    "lidar",
    "radar",
    "vehicle",
    "video",
)

DEP_RE = re.compile(
    r'bazel_dep\(\s*name\s*=\s*"([^"]+)"\s*,\s*version\s*=\s*"([^"]+)"'
)
MODULE_RE = re.compile(
    r'module\(\s*name\s*=\s*"([^"]+)"\s*,\s*version\s*=\s*"([^"]+)"'
)


def main() -> int:
    errors: list[str] = []

    if (ROOT / "MODULE.bazel").exists():
        errors.append("Repository root must not define MODULE.bazel; keep hardware.build under build/.")
    if (ROOT / "build" / "CPPLINT.cfg").exists() or (ROOT / "build" / "tools" / "CPPLINT.cfg").exists():
        errors.append("Starlark-only build/ module must not carry CPPLINT.cfg.")

    for doc_path in (
        "AGENTS.md",
        ".github/copilot-instructions.md",
        ".github/PULL_REQUEST_TEMPLATE.md",
        ".github/workflows/hardware-ci.yml",
        ".agents/skills/README.md",
        ".agents/knowledge/README.md",
        ".agents/notes/README.md",
        "docs/README.md",
    ):
        if not (ROOT / doc_path).is_file():
            errors.append(f"Missing governance/doc file: {doc_path}")

    for mod in MODULES:
        mod_dir = ROOT / mod
        if not mod_dir.is_dir():
            errors.append(f"Missing module directory: {mod}")
            continue

        for required in ("MODULE.bazel", "BUILD.bazel", ".bazelrc", ".bazelrc.dev", ".bazelversion"):
            if not (mod_dir / required).is_file():
                errors.append(f"{mod}: missing {required}")

        bazelversion = mod_dir / ".bazelversion"
        if bazelversion.is_file() and bazelversion.read_text().strip() != "8.8.1":
            errors.append(f"{mod}/.bazelversion must be 8.8.1")

        if mod != "build" and not (mod_dir / "CPPLINT.cfg").is_file():
            errors.append(f"{mod}: missing CPPLINT.cfg")

        module_file = mod_dir / "MODULE.bazel"
        if module_file.is_file():
            text = module_file.read_text()
            m = MODULE_RE.search(text)
            expected_name = f"hardware.{mod}"
            if not m or m.group(1) != expected_name:
                errors.append(f"{mod}/MODULE.bazel must declare name = {expected_name!r}")
            for dep_name, dep_ver in DEP_RE.findall(text):
                if dep_name in EXPECTED_VERSIONS and dep_ver != EXPECTED_VERSIONS[dep_name]:
                    errors.append(
                        f"{mod}/MODULE.bazel: {dep_name}={dep_ver} != expected {EXPECTED_VERSIONS[dep_name]}"
                    )

        build_file = mod_dir / "BUILD.bazel"
        if build_file.is_file():
            build_text = build_file.read_text()
            if mod in RUNTIME_MODULE_REQUIRED and 'name = "module"' not in build_text:
                errors.append(f"{mod}/BUILD.bazel: missing runtime_module target 'module'")
            if mod in COMPONENT_ALIAS_REQUIRED and 'name = "component"' not in build_text:
                errors.append(f"{mod}/BUILD.bazel: missing alias target 'component'")

    equipment_dag = ROOT / "equipment" / "dag" / "equipment.dag"
    if equipment_dag.is_file():
        dag_text = equipment_dag.read_text()
        if "drivers/lib/" in dag_text or "drivers/addition_data/" in dag_text:
            errors.append("equipment/dag/equipment.dag still contains legacy drivers/ paths")

    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        return 1

    print(f"OK: validated {len(MODULES)} hardware modules and repository paradigm files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
