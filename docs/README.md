# WheelOS Hardware Documentation

Formal architecture, customer integration, module authoring, and release
guides for the WheelOS Unified Hardware SDK (`wheelos/hardware`).

- [Architecture Overview](architecture.md): Design principles, 14-module catalog, dependency boundaries, and alignment with `wheelos_core`.
- [Downstream SDK Integration](guides/downstream-integration.md): How external applications consume `hardware.*` modules (C++ SDKs, Cyber components, and `runtime_product` assembly).
- [Driver Benchmark Profiler](guides/driver-benchmark.md): Benchmarking driver CPU, zero-copy memory, I/O, latency baselines, and sensor selection scoring.
- [BCR Release & Source Packaging](guides/bcr-release.md): Multi-module Bzlmod registry publication using a shared Git archive and `strip_prefix`.
- [Module Authoring Guide](guides/module-authoring.md): Standard layout, prefix mapping, build alignment, and target export rules.
