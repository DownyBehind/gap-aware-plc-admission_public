#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import F_now, TransitionParams
from src.sim.cycle_builder import build_cycle


def main() -> None:
    p = TransitionParams()

    # (d) hidden marginal cost is exactly zero: in every hidden cell the SLAC
    # load sits entirely inside the processing gap, so the finish equals the
    # SLAC-free finish of the same N. Also check the derived-field identity
    # auth_span == hidden_len + overflow_len everywhere it is defined.
    for n in range(1, 46):
        for k in range(0, 21):
            cycle = build_cycle(n, k, p)
            assert cycle.auth_span == cycle.hidden_len + cycle.overflow_len
            if cycle.regime == "hidden":
                assert cycle.finish == build_cycle(n, 0, p).finish, f"(d) hidden cell ({n},{k}) has nonzero marginal cost"

    # (e) setup_only: N=0, K>0 has no DC phases and the finish still matches
    # the formula SoT. With the process-floor revision the phantom proc floor is gone:
    # finish(0, K>0) = q*K + B_pkt + B_blk, finish(0, 0) = 0 (empty cycle).
    for k in range(1, 21):
        cycle = build_cycle(0, k, p)
        assert cycle.regime == "setup_only"
        names = [ph.name for ph in cycle.phases]
        assert "DC_REQ" not in names and "DC_RES" not in names
        formula = F_now(0, k, p.C_req_slots, p.C_res_slots, p.C_proc_slots, p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots)
        assert cycle.finish == formula, f"(e) setup_only K={k}: {cycle.finish} != {formula}"
        assert cycle.finish == p.b_slac_slots * k + p.B_pkt_slots + p.B_blk_slots
    assert build_cycle(0, 1, p).finish == 49
    assert build_cycle(0, 0, p).finish == 0

    # (f) K=0: no SLAC_SERVICE/PKT_GUARD phase may exist at all.
    for n in range(0, 46):
        names = [ph.name for ph in build_cycle(n, 0, p).phases]
        assert "SLAC_SERVICE" not in names and "PKT_GUARD" not in names, f"(f) K=0 N={n} emitted a SLAC phase"


if __name__ == "__main__":
    main()
