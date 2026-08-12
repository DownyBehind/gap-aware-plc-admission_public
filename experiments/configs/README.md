# Experiment Configs

> Current paper workflow: see the root README. Paths below describe an earlier layout.

Purpose: parameterize experiments without hard-coding paths in simulator logic.

Main files: `default.json`, `exp1_csma_vs_scheduled.json`, `exp2_fixed_vs_adaptive.json`, `exp3_condA_vs_condAB.json`, `exp4_three_regime.json`.

Use: pass a config to a runner in `experiments/run/`.

Tests: JSON parsing and structure checks.

Expected outputs: none.
