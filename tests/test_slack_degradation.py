#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import F_now, TransitionParams, classify_regime, delta_F_paid


def active_finish(n: int, k: int, p: TransitionParams) -> int:
    return F_now(n, k, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots)


def main() -> None:
    p = TransitionParams()
    assert delta_F_paid(p.C_req_slots, p.C_res_slots, p.b_slac_slots) == 29
    before = active_finish(20, 5, p)
    after = active_finish(21, 4, p)
    assert classify_regime(20, 5, p) == "paid"
    assert after - before == 29
    hidden_before = active_finish(5, 1, p)
    hidden_after = active_finish(6, 0, p)
    assert classify_regime(5, 1, p) == "hidden"
    assert hidden_after - hidden_before in {p.C_res_slots, p.C_res_slots + p.C_req_slots, 13}


if __name__ == "__main__":
    main()
