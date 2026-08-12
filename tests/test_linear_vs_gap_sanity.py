#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, admission_linear, cond_active_state


def n_max(feasible, k: int, lo: int = 20, hi: int = 60) -> int:
    best = 0
    for n in range(lo, hi):
        if feasible(n, k):
            best = n
    return best


def main() -> None:
    # Sanity: in the G(N)=0 region (N >= 20) the gap-aware
    # model reduces to plain capacity arithmetic, so it must agree with the
    # suspension-oblivious linear baseline up to the packetization envelope.
    # The two formulas differ by exactly one B_pkt guard (21 slots) when k > 0:
    #   linear:    36N + 7k + 21            <= T   (single-envelope, B_blk only)
    #   gap-aware: 36N + 7k + 21*[k>0] + 21 <= T   (two-envelope, B_pkt + B_blk)
    # Hence N_max(k) may differ by 0 or 1 depending on where the 21-slot guard
    # falls relative to the 36-slot per-EV step, and gap-aware is never the
    # larger one (two-envelope accounting is the more conservative side).
    # Do NOT tighten this to exact equality.
    p = TransitionParams()
    for k in range(0, 13):
        lin = n_max(lambda n, kk: admission_linear(n, kk, p), k)
        gap = n_max(lambda n, kk: cond_active_state(n, kk, p), k)
        assert lin >= 20 and gap >= 20, f"k={k}: N_max left the G(N)=0 region"
        assert gap <= lin, f"k={k}: gap-aware admitted more than linear ({gap} > {lin})"
        assert lin - gap in {0, 1}, f"k={k}: N_max diff {lin - gap} outside {{0, 1}}"
    # k=0: no SLAC frame -> no B_pkt guard -> the two models agree exactly.
    assert n_max(lambda n, kk: admission_linear(n, kk, p), 0) == n_max(
        lambda n, kk: cond_active_state(n, kk, p), 0
    )


if __name__ == "__main__":
    main()
