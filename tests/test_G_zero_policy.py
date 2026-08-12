#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import G_processing_gap, TransitionParams, classify_regime


def main() -> None:
    p = TransitionParams()
    try:
        G_processing_gap(0, p.C_req_slots, p.C_proc_slots)
    except ValueError:
        pass
    else:
        raise AssertionError("G(0) must not be used for hidden/paid classification")
    assert classify_regime(0, 2, p) == "setup_only"


if __name__ == "__main__":
    main()
