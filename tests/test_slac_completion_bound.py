#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, q_slac, slac_completion_bound_ms


def main() -> None:
    p = TransitionParams()
    q = q_slac(p.C_slac_slots, p.b_slac_slots)
    assert q == 36
    bound = slac_completion_bound_ms(q, p.T_ctrl_ms)
    assert bound == 1850
    assert bound <= p.D_slac_ms
    credit = 0
    for _ in range(q):
        credit += p.b_slac_slots
    assert credit >= p.C_slac_slots


if __name__ == "__main__":
    main()
