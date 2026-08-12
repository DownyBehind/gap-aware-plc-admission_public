# Experiments

> Current paper workflow: see the root README. Paths below describe an earlier layout.

Purpose: reproducible experiment configuration, execution, and analysis.

Main folders: `configs/`, `run/`, `analysis/`, `expected/`.

Use: edit JSON configs, run shell scripts, then generate plots from saved results.

Tests: `tests/run_all_tests.sh` syntax-checks scripts and runs a smoke experiment when ns-3 is available.

Expected outputs: `results/<experiment>/{config.json,metrics.csv,events.csv,summary.json,figures/}`.
