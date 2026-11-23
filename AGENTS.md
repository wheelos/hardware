# Agent Guide

## Rules

- Read the relevant `MODULE.bazel`, `BUILD.bazel`/`BUILD`, source, and tests
  before making changes.
- Keep `pkgs/hardware` as a multi-module Bzlmod Monorepo: each top-level module
  directory is an independent `MODULE.bazel` root; do **not** flatten
  `hardware.build` (`build/`) into the repository root.
- Preserve module dependency isolation: never introduce cross-driver or
  unnecessary algorithm/transform dependencies into unrelated hardware modules
  (for example, `hardware.camera` must not pull `wheelos_transform`,
  `hardware.canbus`, or LiDAR SDKs).
- Keep the existing `hardware.hal` (`Stream`) and `hardware.canbus`
  (`CanClient`) contracts separate unless explicitly asked to unify them.
- Run Bazel builds and tests inside the managed development container as the
  mapped non-root user, reusing `/apollo/.cache/bazel` caches.
- Read the matching `.agents/skills/*/SKILL.md` before building, onboarding a
  module, publishing to BCR, or reviewing changes.
- Keep durable facts in `.agents/knowledge/`, formal user-facing docs in
  `docs/`, and temporary investigation notes in `.agents/notes/`.

## Commands

- Validate multi-module structure and version alignment:
  `python3 scripts/validate_modules.py`
- Validate an isolated downstream consumer (run inside dev container):
  `bash scripts/validate_downstream_consumer.sh`
- Build and test a single module (from `<module>/`, inside dev container,
  using `bazelisk` so `.bazelversion` 8.8.1 is selected):
  `bazelisk test --config=dev //...`

## Directory layout

```text
├── AGENTS.md         ← Agent entry point and working rules
├── .github/          ← GitHub governance, Copilot instructions, PR template, CI
├── .agents/
│   ├── skills/       ← Task workflows (how-to)
│   ├── knowledge/    ← Durable repository architecture and conventions
│   └── notes/        ← Ignored temporary investigation notes
└── docs/             ← Formal user and SDK integration documentation
```

- Knowledge index: `.agents/knowledge/README.md`
- Skills index: `.agents/skills/README.md`
- Documentation index: `docs/README.md`
