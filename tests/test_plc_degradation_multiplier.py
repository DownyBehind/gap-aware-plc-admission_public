#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, degraded_params


def main() -> None:
    p = TransitionParams()
    expected = {
        1.0: (15, 21),
        1.25: (19, 27),
        1.5: (23, 32),
        2.0: (30, 42),
    }
    for factor, values in expected.items():
        d = degraded_params(p, factor)
        assert (d.C_req_slots, d.C_res_slots) == values


if __name__ == "__main__":
    main()
