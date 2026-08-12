#!/usr/bin/env python3
"""Generate formula-sanity/mock artifacts only.

These artifacts are intentionally excluded from the final ns-3 evaluation.
They must never be reported as actual ns-3 simulation results.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "results" / "formula_sanity"


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    summary = {
        "result_type": "formula_sanity_or_mock",
        "simulator": "none",
        "fallback_used": True,
        "not_ns3_result": True,
        "status": "EXCLUDED_FROM_FINAL_NS3_EVALUATION",
        "note": "The previous synthetic final generator was isolated. Final paper results must come from results/ns3_final only.",
    }
    (OUT / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")


if __name__ == "__main__":
    main()
