#!/usr/bin/env python3
"""Single source-of-truth formulas for transition-aware admission.

Single source of truth used by tests, final experiment generation,
plotting metadata, and report generation.
"""

from __future__ import annotations

from dataclasses import dataclass
from math import ceil
from typing import Any, Mapping


@dataclass(frozen=True)
class TransitionParams:
    T_ctrl_ms: float = 50.0
    slot_duration_us: float = 35.84
    T_ctrl_slots: int = 1395
    # O_map is retained as the constant term c_0 of C_bcn(N,k) = c_0 + c_e*(N + s(k));
    # the nominal configuration runs with O_map = 0 so that admission is
    # checked against T = 1395 directly.
    O_map_slots: int = 0
    C_req_slots: int = 15
    C_res_slots: int = 21
    C_proc_slots: int = 280
    b_slac_slots: int = 7
    B_pkt_slots: int = 21
    B_blk_slots: int = 21
    C_slac_slots: int = 247
    D_slac_ms: float = 2000.0

    @classmethod
    def from_mapping(cls, data: Mapping[str, Any]) -> "TransitionParams":
        return cls(
            T_ctrl_ms=float(data.get("T_ctrl_ms", cls.T_ctrl_ms)),
            slot_duration_us=float(data.get("slot_duration_us", cls.slot_duration_us)),
            T_ctrl_slots=int(data.get("T_ctrl_slots", cls.T_ctrl_slots)),
            O_map_slots=int(data.get("O_map_slots", cls.O_map_slots)),
            C_req_slots=int(data.get("C_req_slots", cls.C_req_slots)),
            C_res_slots=int(data.get("C_res_slots", cls.C_res_slots)),
            C_proc_slots=int(data.get("C_proc_slots", cls.C_proc_slots)),
            b_slac_slots=int(data.get("b_slac_slots", cls.b_slac_slots)),
            B_pkt_slots=int(data.get("B_pkt_slots", cls.B_pkt_slots)),
            B_blk_slots=int(data.get("B_blk_slots", cls.B_blk_slots)),
            C_slac_slots=int(data.get("C_slac_slots", cls.C_slac_slots)),
            D_slac_ms=float(data.get("D_slac_ms", cls.D_slac_ms)),
        )

    @property
    def T_sched_slots(self) -> int:
        return t_sched(self.T_ctrl_slots, self.O_map_slots)

    def snapshot(self) -> dict[str, Any]:
        return {
            "T_ctrl_ms": self.T_ctrl_ms,
            "slot_duration_us": self.slot_duration_us,
            "T_ctrl_slots": self.T_ctrl_slots,
            "O_map_slots": self.O_map_slots,
            "T_sched_slots": self.T_sched_slots,
            "C_req_slots": self.C_req_slots,
            "C_res_slots": self.C_res_slots,
            "C_proc_slots": self.C_proc_slots,
            "b_slac_slots": self.b_slac_slots,
            "B_pkt_slots": self.B_pkt_slots,
            "B_blk_slots": self.B_blk_slots,
            "C_slac_slots": self.C_slac_slots,
            "D_slac_ms": self.D_slac_ms,
        }


def t_sched(T_ctrl_slots: int, O_map: int) -> int:
    return T_ctrl_slots - O_map


def F_DC(N: int, C_req: int, C_res: int, C_proc: int, B_blk: int) -> int:
    return max(N * C_req, C_req + C_proc) + N * C_res + B_blk


def G_processing_gap(N: int, C_req: int, C_proc: int) -> int:
    if N < 1:
        raise ValueError("G(N) is defined only for N >= 1; use setup_only regime for N=0.")
    return max(0, C_proc - (N - 1) * C_req)


def F_active_state(
    N: int,
    K_active: int,
    C_req: int,
    C_res: int,
    C_proc: int,
    b_slac: int,
    B_pkt: int,
    B_blk: int,
) -> int:
    if N == 0 and K_active == 0:
        # Empty cycle: there is no frame for the carry-in envelope (B_blk)
        # to delay, so the finish is defined as 0.
        return 0
    pkt = B_pkt if K_active > 0 else 0
    # The process term is the earliest start of the first DC response, so it
    # applies only while a DC stream exists. Keeping the floor at N=0 would
    # charge processing for a stream that is not there and contradict the
    # slack definition slack(0) ~= T, i.e. F(0) ~= 0, under which the whole
    # channel opens to authentication.
    process = C_req + C_proc if N > 0 else 0
    return max(N * C_req + b_slac * K_active + pkt, process) + N * C_res + B_blk


def F_now(
    N: int,
    K_after_admit: int,
    C_req: int,
    C_res: int,
    C_proc: int,
    b_slac: int,
    B_pkt: int,
    B_blk: int,
) -> int:
    return F_active_state(N, K_after_admit, C_req, C_res, C_proc, b_slac, B_pkt, B_blk)


def F_candidate_admission(N: int, K_current: int, params: TransitionParams) -> int:
    return F_active_state(
        N,
        K_current + 1,
        params.C_req_slots,
        params.C_res_slots,
        params.C_proc_slots,
        params.b_slac_slots,
        params.B_pkt_slots,
        params.B_blk_slots,
    )


def F_future(N_future: int, C_req: int, C_res: int, C_proc: int, B_blk: int) -> int:
    return F_DC(N_future, C_req, C_res, C_proc, B_blk)


def cond_active_state(N: int, K_active: int, params: TransitionParams) -> bool:
    return F_active_state(N, K_active, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots) <= params.T_sched_slots


def cond_candidate_A(N: int, K_current: int, params: TransitionParams) -> bool:
    return F_candidate_admission(N, K_current, params) <= params.T_sched_slots


def cond_A(N: int, K: int, params: TransitionParams) -> bool:
    return cond_candidate_A(N, K, params)


def cond_candidate_B(N: int, K_current: int, params: TransitionParams) -> bool:
    return F_future(N + K_current + 1, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.B_blk_slots) <= params.T_sched_slots


def cond_B(N: int, K: int, params: TransitionParams) -> bool:
    return cond_candidate_B(N, K, params)


def cond_post_transition_state(N: int, K_active: int, params: TransitionParams) -> bool:
    return F_future(N + K_active, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.B_blk_slots) <= params.T_sched_slots


def admit(N: int, K: int, params: TransitionParams) -> bool:
    return admit_candidate(N, K, params)


def admit_candidate(N: int, K_current: int, params: TransitionParams) -> bool:
    return cond_candidate_A(N, K_current, params) and cond_candidate_B(N, K_current, params) and slac_completion_bound_ms(q_slac(params.C_slac_slots, params.b_slac_slots), params.T_ctrl_ms) <= params.D_slac_ms


def admission_linear(N: int, k: int, params: TransitionParams) -> bool:
    """Suspension-oblivious single-envelope baseline (kept for comparison).

    C_dc*N + q*k + B_blk <= T with C_dc = C_req + C_res
    (36N + 7k + 21 <= 1395 at the defaults). Comparison-only: this form
    erases the processing-gap structure, so it must never replace the
    gap-aware model.
    """
    C_dc = params.C_req_slots + params.C_res_slots
    return C_dc * N + params.b_slac_slots * k + params.B_blk_slots <= params.T_sched_slots


def N_star(C_req: int, C_proc: int) -> int:
    return 1 + ceil(C_proc / C_req)


def delta_F_paid(C_req: int, C_res: int, b_slac: int) -> int:
    return C_req + C_res - b_slac


def q_slac(C_slac: int, b_slac: int) -> int:
    # Packetization debt (B_pkt) is excluded: it is bounded by one frame envelope
    # under debt-neutral packetization and absorbed by the +T rounding
    # margin in the completion bound below.
    return ceil(C_slac / b_slac)


def slac_completion_bound_ms(q_slac_value: int, T_ctrl_ms: float) -> float:
    # Admission-time per-session multiple check, L_init-inclusive:
    # (ceil(C_slac/q) + 1) * T  =  37T = 1850 ms <= D_slac = 2000 ms at defaults.
    return (q_slac_value + 1) * T_ctrl_ms


def classify_regime(N: int, K: int, params: TransitionParams) -> str:
    if N == 0:
        return "setup_only"
    if not cond_active_state(N, K, params) or not cond_post_transition_state(N, K, params):
        return "rejected"
    auth_with_pkt = params.b_slac_slots * K + (params.B_pkt_slots if K > 0 else 0)
    return "hidden" if auth_with_pkt <= G_processing_gap(N, params.C_req_slots, params.C_proc_slots) else "paid"


def degraded_params(params: TransitionParams, factor: float) -> TransitionParams:
    return TransitionParams(
        T_ctrl_ms=params.T_ctrl_ms,
        slot_duration_us=params.slot_duration_us,
        T_ctrl_slots=params.T_ctrl_slots,
        O_map_slots=params.O_map_slots,
        C_req_slots=ceil(params.C_req_slots * factor),
        C_res_slots=ceil(params.C_res_slots * factor),
        C_proc_slots=params.C_proc_slots,
        b_slac_slots=params.b_slac_slots,
        B_pkt_slots=params.B_pkt_slots,
        B_blk_slots=params.B_blk_slots,
        C_slac_slots=params.C_slac_slots,
        D_slac_ms=params.D_slac_ms,
    )


def enforce_response_dominant(params: TransitionParams) -> None:
    if params.C_res_slots < params.C_req_slots:
        raise ValueError("Closed-form bound requires C_res >= C_req; use recurrence-based simulator check for this config.")
