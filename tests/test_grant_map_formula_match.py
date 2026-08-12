#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import F_now, TransitionParams
from src.sim.cycle_builder import build_cycle


def cpp_compute_finish_time(n: int, k: int, p: TransitionParams) -> int:
    # Transcription of GrantMapScheduler::ComputeFinishTime
    # (ns3/contrib/ev-plc-transition/model/grant-map-scheduler.cc), current form:
    # empty cycle -> 0; process floor only while a DC stream exists.
    if n == 0 and k == 0:
        return 0
    auth_end = n * p.C_req_slots + p.b_slac_slots * k + (p.B_pkt_slots if k > 0 else 0)
    proc_floor = p.C_req_slots + p.C_proc_slots if n > 0 else 0
    response_start = max(auth_end, proc_floor)
    return response_start + n * p.C_res_slots + p.B_blk_slots


def pre_d2_formula(n: int, k: int, p: TransitionParams) -> int:
    # Legacy F_active_state with the unconditional process floor; kept only as
    # the regression reference proving the revision changed no N >= 1 cell.
    pkt = p.B_pkt_slots if k > 0 else 0
    return max(n * p.C_req_slots + p.b_slac_slots * k + pkt, p.C_req_slots + p.C_proc_slots) + n * p.C_res_slots + p.B_blk_slots


def main() -> None:
    p = TransitionParams()
    # Invariants (a) and (b) over the full grid: the cycle builder, the
    # formula SoT, and the C++ scheduler recurrence must be one and the same.
    for n in range(0, 46):
        for k in range(0, 21):
            cycle = build_cycle(n, k, p)
            formula = F_now(n, k, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots)
            assert cycle.finish == formula, f"(a) builder vs formula at ({n},{k}): {cycle.finish} != {formula}"
            assert cycle.finish == cpp_compute_finish_time(n, k, p), f"(b) builder vs C++ recurrence at ({n},{k})"
            # Regression guard: every N >= 1 cell must be bit-identical to
            # the legacy formula.
            if n >= 1:
                assert formula == pre_d2_formula(n, k, p), f"process-floor regression at ({n},{k})"


if __name__ == "__main__":
    main()
