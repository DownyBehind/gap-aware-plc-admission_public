#!/usr/bin/env python3
"""Layer-1 cycle builder: requests-first grant-map phase decomposition.

Mirrors GrantMapScheduler::ComputeFinishTime/BuildGrantMap
(ns3/contrib/ev-plc-transition/model/grant-map-scheduler.cc) under the
two-envelope accounting: PKT_GUARD is the in-map budget-overrun envelope of
the last non-preemptive SLAC frame (B_pkt), while the out-of-map carry-in
envelope (B_blk) extends the finish bound and is never a scheduled phase.
Regime classification is delegated to formulas.classify_regime — no
reimplementation here.
"""

from __future__ import annotations

from dataclasses import dataclass

from src.formulas.transition_formulas import (
    G_processing_gap,
    TransitionParams,
    classify_regime,
)


@dataclass(frozen=True)
class Phase:
    name: str
    start: int
    length: int

    @property
    def end(self) -> int:
        return self.start + self.length


@dataclass(frozen=True)
class Cycle:
    N: int
    K: int
    phases: tuple[Phase, ...]
    auth_span: int
    hidden_len: int
    overflow_len: int
    finish: int
    slack: int
    regime: str


def build_cycle(N: int, K: int, params: TransitionParams) -> Cycle:
    req_len = N * params.C_req_slots
    slac_len = params.b_slac_slots * K
    guard_len = params.B_pkt_slots if K > 0 else 0
    auth_span = slac_len + guard_len

    phases: list[Phase] = []
    if N > 0:
        phases.append(Phase("DC_REQ", 0, req_len))
    cursor = req_len
    if K > 0:
        phases.append(Phase("SLAC_SERVICE", cursor, slac_len))
        cursor += slac_len
        phases.append(Phase("PKT_GUARD", cursor, guard_len))
        cursor += guard_len
    # The process floor is the earliest start of the first DC response, so it
    # applies only when a DC stream exists.
    proc_floor = params.C_req_slots + params.C_proc_slots if N > 0 else 0
    response_start = max(cursor, proc_floor)
    if N > 0:
        phases.append(Phase("DC_RES", response_start, N * params.C_res_slots))
    if N == 0 and K == 0:
        # Empty cycle: no frame for the carry-in envelope (B_blk) to delay.
        finish = 0
    else:
        finish = response_start + N * params.C_res_slots + params.B_blk_slots

    if N >= 1:
        gap = G_processing_gap(N, params.C_req_slots, params.C_proc_slots)
        hidden_len = min(auth_span, gap)
        overflow_len = auth_span - hidden_len
    else:
        # setup_only: no DC deadline to hide behind or to push, so the
        # hidden/overflow split is undefined; report zeros.
        hidden_len = 0
        overflow_len = 0

    return Cycle(
        N=N,
        K=K,
        phases=tuple(phases),
        auth_span=auth_span,
        hidden_len=hidden_len,
        overflow_len=overflow_len,
        finish=finish,
        slack=params.T_sched_slots - finish,
        regime=classify_regime(N, K, params),
    )
