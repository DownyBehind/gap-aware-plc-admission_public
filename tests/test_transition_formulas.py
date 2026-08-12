#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import (
    F_DC,
    F_now,
    TransitionParams,
    G_processing_gap,
    N_star,
    cond_A,
    cond_B,
    delta_F_paid,
    q_slac,
    slac_completion_bound_ms,
    t_sched,
)


def main() -> None:
    p = TransitionParams()
    assert t_sched(1395, 0) == 1395
    assert p.T_sched_slots == 1395
    assert G_processing_gap(1, p.C_req_slots, p.C_proc_slots) == 280
    assert G_processing_gap(20, p.C_req_slots, p.C_proc_slots) == 0
    assert N_star(p.C_req_slots, p.C_proc_slots) == 20
    assert delta_F_paid(p.C_req_slots, p.C_res_slots, p.b_slac_slots) == 29
    q = q_slac(p.C_slac_slots, p.b_slac_slots)
    assert q == 36
    assert slac_completion_bound_ms(q, p.T_ctrl_ms) == 1850
    assert slac_completion_bound_ms(q, p.T_ctrl_ms) < p.D_slac_ms
    expected = {
        1: max(15, 295) + 21 + 21,
        10: max(150, 295) + 210 + 21,
        20: max(300, 295) + 420 + 21,
        36: max(540, 295) + 756 + 21,
    }
    for n, value in expected.items():
        assert F_DC(n, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.B_blk_slots) == value
    assert F_now(36, 2, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots) <= p.T_sched_slots
    # Counterexample state under T_sched=1395, q=7: Cond A passes, Cond B rejects.
    assert cond_A(37, 1, p) is True
    assert cond_B(37, 1, p) is False
    assert cond_B(36, 1, p) is True


if __name__ == "__main__":
    main()
