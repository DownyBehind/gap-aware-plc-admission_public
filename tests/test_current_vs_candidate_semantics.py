#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, cond_active_state, cond_candidate_A, F_active_state, F_candidate_admission, classify_regime


def main() -> None:
    p = TransitionParams()
    assert F_candidate_admission(10, 0, p) == F_active_state(10, 1, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots)
    assert cond_candidate_A(10, 0, p) == cond_active_state(10, 1, p)
    assert classify_regime(0, 1, p) == "setup_only"


if __name__ == "__main__":
    main()
