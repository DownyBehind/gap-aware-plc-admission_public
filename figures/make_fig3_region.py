#!/usr/bin/env python3
"""Fig.3 admission region — thin wrapper around the Layer-1 sweep.

Runs experiments/layer1/sweep_admission_region.py with its output
redirected to figures/out/, then asserts that the regenerated
admission_region.csv is identical to the committed copy in
results/layer1/ (the figure may only draw committed numbers)."""
import filecmp
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "figures" / "out"
sys.path.insert(0, str(ROOT))

import experiments.layer1.sweep_admission_region as sweep_mod

OUT_DIR.mkdir(parents=True, exist_ok=True)
sweep_mod.OUT_DIR = OUT_DIR
sweep_mod.main()

regen = OUT_DIR / "admission_region.csv"
committed = ROOT / "results" / "layer1" / "admission_region.csv"
if not filecmp.cmp(regen, committed, shallow=False):
    raise SystemExit("FAIL: regenerated admission_region.csv differs from the committed copy")
print("admission_region.csv identical to committed copy")

src = OUT_DIR / "fig_admission_region.pdf"
shutil.copyfile(src, OUT_DIR / "fig3_admission_region.pdf")
shutil.copyfile(OUT_DIR / "fig_admission_region.png", OUT_DIR / "fig3_admission_region.png")
print(f"fig: {OUT_DIR / 'fig3_admission_region.pdf'}")
