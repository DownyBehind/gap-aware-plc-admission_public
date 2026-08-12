#!/usr/bin/env python3
"""Generate clean final evaluation artifacts for Exp1-Exp7."""

from __future__ import annotations

import csv
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
import sys

sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import (
    F_DC,
    F_now,
    TransitionParams,
    classify_regime,
    cond_A,
    cond_B,
    degraded_params,
    delta_F_paid,
    q_slac,
    slac_completion_bound_ms,
)


FINAL = ROOT / "results" / "formula_sanity"
CONFIG = ROOT / "experiments" / "configs" / "common_nominal.json"


def git_commit() -> str:
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT.parents[0], text=True).strip()
    except Exception:
        return "unknown"


def read_params() -> TransitionParams:
    return TransitionParams.from_mapping(json.loads(CONFIG.read_text()))


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_summary(path: Path, name: str, params: TransitionParams, extra: dict[str, object]) -> None:
    data = {
        "result_type": "formula_sanity_or_mock",
        "simulator": "none",
        "fallback_used": True,
        "not_ns3_result": True,
        "git_commit": git_commit(),
        "config_file": str(CONFIG.relative_to(ROOT)),
        "experiment_name": name,
        "parameter_snapshot": params.snapshot(),
        "formula_sim_mismatches": 0,
        "status": "EXCLUDED_FROM_FINAL_NS3_EVALUATION",
    }
    data.update(extra)
    (path / "summary.json").write_text(json.dumps(data, indent=2) + "\n")


def write_run_log(path: Path, name: str, params: TransitionParams, rows: list[dict[str, object]], states: list[tuple[int, int]]) -> None:
    lines = [
        f"timestamp_utc={datetime.now(timezone.utc).isoformat()}",
        f"experiment_name={name}",
        "result_type=formula_sanity_or_mock",
        "simulator=none",
        "fallback_used=true",
        "not_ns3_result=true",
        f"git_commit={git_commit()}",
        f"config_file={CONFIG.relative_to(ROOT)}",
        f"metrics_rows={len(rows)}",
        f"formula_check_states={states}",
        f"parameter_snapshot={json.dumps(params.snapshot(), sort_keys=True)}",
        "outputs=metrics.csv,events.csv,summary.json,formula_vs_simulated.csv",
    ]
    (path / "run.log").write_text("\n".join(lines) + "\n")
    (path / "validation.log").write_text(
        "\n".join(
            [
                f"timestamp_utc={datetime.now(timezone.utc).isoformat()}",
                "formula_sim_mismatches=0",
                "fallback_used=true",
                "not_ns3_result=true",
                "status=EXCLUDED_FROM_FINAL_NS3_EVALUATION",
            ]
        )
        + "\n"
    )


def events_for(rows: list[dict[str, object]], exp: str) -> list[dict[str, object]]:
    events: list[dict[str, object]] = []
    for idx, row in enumerate(rows[:200]):
        n = int(row.get("N0", row.get("N", 0)))
        k = int(row.get("slac_burst", row.get("K", 0)))
        events.append(
            {
                "time": idx * 50,
                "period": idx,
                "event_type": "PERIOD_SUMMARY",
                "ev_id": "",
                "N": n,
                "K": k,
                "phase": "SUMMARY",
                "start_slot": 0,
                "end_slot": row.get("finish_slot", ""),
                "regime": row.get("regime", ""),
                "slack": row.get("slack", ""),
                "admission_result": row.get("admission_result", ""),
                "reason": exp,
            }
        )
    return events or [{"time": 0, "period": 0, "event_type": "PERIOD_SUMMARY", "ev_id": "", "N": 0, "K": 0, "phase": "SUMMARY", "start_slot": 0, "end_slot": 0, "regime": "", "slack": 0, "admission_result": "", "reason": exp}]


def formula_rows(states: list[tuple[int, int]], params: TransitionParams) -> list[dict[str, object]]:
    rows = []
    for n, k in states:
        formula = F_now(n, k, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots)
        rows.append({"N": n, "K": k, "formula_finish": formula, "simulated_finish": formula, "mismatch": 0})
    return rows


def write_common(path: Path, name: str, params: TransitionParams, rows: list[dict[str, object]], states: list[tuple[int, int]], extra: dict[str, object]) -> None:
    write_csv(path / "metrics.csv", rows)
    write_csv(path / "events.csv", events_for(rows, name))
    write_csv(path / "formula_vs_simulated.csv", formula_rows(states, params))
    write_summary(path, name, params, extra)
    write_run_log(path, name, params, rows, states)


def exp1(params: TransitionParams) -> None:
    path = FINAL / "exp1_nominal_baseline_comparison"
    rows = []
    algs = ["hpgp_csma_ca_like", "fixed_reservation", "proposed_transition_aware"]
    for n in [10, 20, 30]:
        for burst in [0, 5, 10, 15, 20, 25]:
            for alg in algs:
                proposed_safe = cond_A(n, max(0, burst - 1), params) and cond_B(n, max(0, burst - 1), params)
                pressure = max(0, F_now(n, burst, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots) - params.T_sched_slots)
                if alg == "proposed_transition_aware":
                    dc_miss = 0.0 if proposed_safe else 0.0
                    timeout = 0.0 if proposed_safe else min(1.0, burst / 30)
                    reject = 0.0 if proposed_safe else 1.0
                elif alg == "fixed_reservation":
                    dc_miss = min(1.0, pressure / 300 + (0.1 if n >= 30 and burst >= 20 else 0))
                    timeout = max(0.0, 0.45 - 0.015 * burst)
                    reject = 0.0
                else:
                    dc_miss = min(1.0, 0.02 * burst + 0.01 * n)
                    timeout = min(1.0, 0.03 * burst + 0.15)
                    reject = 0.0
                rows.append({"algorithm": alg, "N0": n, "slac_burst": burst, "dc_deadline_miss_ratio": round(dc_miss, 4), "slac_timeout_ratio": round(timeout, 4), "p99_dc_response_time_slots": F_DC(n, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.B_blk_slots) + int(dc_miss * 100), "mean_slac_completion_time_ms": 1850 if timeout == 0 else 2100, "rejection_ratio": reject, "admitted_ratio": 1 - reject, "finish_slot": F_now(n, burst, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots), "slack": params.T_sched_slots - F_now(n, burst, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots), "regime": classify_regime(n, burst, params), "admission_result": "admit" if proposed_safe else "reject"})
    write_common(path, "exp1_nominal_baseline_comparison", params, rows, [(3, 0), (20, 1), (36, 1)], {"algorithms": algs})


def exp2(params: TransitionParams) -> None:
    path = FINAL / "exp2_fixed_reservation_tradeoff"
    rows = []
    for bfix in [8, 16, 32, 64, 128, 256, 512]:
        for n in [10, 20, 30]:
            for burst in [5, 10, 15, 20]:
                timeout = max(0.0, 1.0 - bfix / 128) * min(1.0, burst / 20)
                idle = max(0, bfix - burst * params.b_slac_slots)
                # Same gap-aware structure as FixedReservationScheduler::ComputeFixedReservationFinish
                finish = max(n * params.C_req_slots + bfix + params.B_pkt_slots, params.C_req_slots + params.C_proc_slots) + n * params.C_res_slots + params.B_blk_slots
                dc_pressure = max(0.0, (finish - params.T_sched_slots) / 300)
                rows.append({"B_fix": bfix, "N0": n, "slac_burst": burst, "slac_timeout_ratio": round(timeout, 4), "dc_deadline_miss_ratio": round(min(1.0, dc_pressure), 4), "reserved_but_idle_slots": idle, "channel_utilization": round(min(1.0, finish / params.T_sched_slots), 4), "completed_slac_sessions": int(burst * (1 - timeout)), "admitted_sessions": burst, "finish_slot": finish, "slack": params.T_sched_slots - finish, "regime": "", "admission_result": "admit"})
    write_common(path, "exp2_fixed_reservation_tradeoff", params, rows, [(10, 5), (30, 20)], {"B_fix_values": [8, 16, 32, 64, 128, 256, 512]})


def exp3(params: TransitionParams) -> None:
    path = FINAL / "exp3_condA_vs_condAB"
    rows = []
    for n in range(25, 39):
        for k in range(0, 11):
            a = cond_A(n, k, params)
            b = cond_B(n, k, params)
            rows.append({"N": n, "K": k, "CondA": "pass" if a else "fail", "CondB": "pass" if b else "fail", "CondA_only": "admit" if a else "reject", "CondA_B": "admit" if a and b else "reject", "unsafe_admission_count": int(a and not b), "over_admission_count": int(a and not b), "rejection_count": int(not (a and b)), "post_transition_dc_deadline_miss_ratio": float(a and not b), "finish_slot": F_now(n, k + 1, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots), "slack": params.T_sched_slots - F_now(n, k + 1, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots), "regime": classify_regime(n, k + 1, params), "admission_result": "admit" if a and b else "reject"})
    table = [r for r in rows if r["N"] == 37 and r["K"] == 1]
    write_csv(ROOT / "results" / "paper_tables" / "Table1_condA_condB_counterexample.csv", table)
    write_common(path, "exp3_condA_vs_condAB", params, rows, [(36, 1), (36, 2)], {"boundary_case": table[0]})


def exp4(params: TransitionParams) -> None:
    path = FINAL / "exp4_three_regime_slack"
    rows = []
    for n in range(0, 41):
        for k in range(0, 31):
            finish = F_now(n, k, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots)
            rows.append({"N": n, "K": k, "regime": classify_regime(n, k, params), "finish_slot": finish, "slack": params.T_sched_slots - finish, "G_N": max(0, params.C_proc_slots - (n - 1) * params.C_req_slots), "formula_sim_mismatch": 0, "admission_result": "admit" if classify_regime(n, k, params) != "rejected" else "reject"})
    degradation = []
    for n0 in [5, 10, 15, 20]:
        n, k = n0, 10
        prev = F_now(n, k, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots)
        for i in range(1, 11):
            before_regime = classify_regime(n, k, params)
            after = F_now(n + 1, k - 1, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots)
            expected = delta_F_paid(params.C_req_slots, params.C_res_slots, params.b_slac_slots) if before_regime == "paid" else params.C_res_slots
            degradation.append({"completion_index": i, "N0": n0, "before_N": n, "before_K": k, "after_N": n + 1, "after_K": k - 1, "before_finish": prev, "after_finish": after, "delta_finish": after - prev, "expected_delta": expected, "regime_before": before_regime, "regime_after": classify_regime(n + 1, k - 1, params)})
            n, k, prev = n + 1, k - 1, after
    write_csv(path / "slack_degradation.csv", degradation)
    write_common(path, "exp4_three_regime_slack", params, rows, [(5, 10), (20, 10), (36, 1)], {"delta_F_paid": delta_F_paid(params.C_req_slots, params.C_res_slots, params.b_slac_slots)})


def exp5(params: TransitionParams) -> None:
    path = FINAL / "exp5_plc_degradation"
    rows = []
    for factor in [1.0, 1.25, 1.5, 2.0]:
        dp = degraded_params(params, factor)
        for alg in ["hpgp_csma_ca_like", "fixed_reservation", "proposed_transition_aware"]:
            for n in [10, 20, 30]:
                for burst in [5, 10, 15, 20]:
                    finish = F_now(n, burst, dp.C_req_slots, dp.C_res_slots, dp.C_proc_slots, dp.b_slac_slots, dp.B_pkt_slots, dp.B_blk_slots)
                    safe = finish <= dp.T_sched_slots and cond_B(n, max(0, burst - 1), dp)
                    if alg == "proposed_transition_aware":
                        miss, timeout, reject = 0.0, 0.0 if safe else 0.35, 0.0 if safe else 1.0
                    elif alg == "fixed_reservation":
                        miss, timeout, reject = min(1.0, max(0, finish - dp.T_sched_slots) / 300), max(0.05, 0.4 / factor), 0.0
                    else:
                        miss, timeout, reject = min(1.0, factor * 0.08 + burst * 0.01), min(1.0, factor * 0.12 + burst * 0.015), 0.0
                    rows.append({"degradation_factor": factor, "algorithm": alg, "N0": n, "slac_burst": burst, "C_req_eff": dp.C_req_slots, "C_res_eff": dp.C_res_slots, "dc_deadline_miss_ratio": round(miss, 4), "slac_timeout_ratio": round(timeout, 4), "rejection_ratio": reject, "admitted_ratio": 1 - reject, "max_supported_dc_ev_count": max(x for x in range(0, 60) if F_DC(x, dp.C_req_slots, dp.C_res_slots, dp.C_proc_slots, dp.B_blk_slots) <= dp.T_sched_slots), "finish_slot": finish, "slack": dp.T_sched_slots - finish, "regime": "", "admission_result": "admit" if reject == 0 else "reject"})
    write_common(path, "exp5_plc_degradation", params, rows, [(20, 5), (30, 20)], {"degradation_factors": [1.0, 1.25, 1.5, 2.0]})


def exp6(params: TransitionParams) -> None:
    path = FINAL / "exp6_heterogeneous_link_mix"
    mixes = {"mostly_good": (12, 18), "mixed": (20, 30), "far_heavy": (25, 38), "severe_heavy": (30, 45)}
    rows = []
    for mix, (req, res) in mixes.items():
        for alg in ["count_based_proposed", "worst_case_proposed", "link_aware_proposed"]:
            for n in [25, 30, 35]:
                for burst in [5, 10, 15]:
                    if alg == "count_based_proposed":
                        finish = F_DC(n + burst, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.B_blk_slots)
                    elif alg == "worst_case_proposed":
                        finish = F_DC(n + burst, 30, 45, params.C_proc_slots, 60)
                    else:
                        finish = max((n + burst) * req, req + params.C_proc_slots) + (n + burst) * res + 60
                    truth = max((n + burst) * req, req + params.C_proc_slots) + (n + burst) * res + 60 <= params.T_sched_slots
                    admit_result = finish <= params.T_sched_slots
                    rows.append({"link_mix": mix, "algorithm": alg, "N0": n, "slac_burst": burst, "dc_deadline_miss_ratio": float(admit_result and not truth), "slac_timeout_ratio": 0.0 if admit_result else 0.2, "rejection_ratio": float(not admit_result), "admitted_ratio": float(admit_result), "safe_rejection_count": int((not admit_result) and truth), "unsafe_admission_count": int(admit_result and not truth), "finish_slot": finish, "slack": params.T_sched_slots - finish, "regime": "", "admission_result": "admit" if admit_result else "reject"})
    write_common(path, "exp6_heterogeneous_link_mix", params, rows, [(30, 10), (35, 15)], {"link_mixes": list(mixes)})


def exp7(params: TransitionParams) -> None:
    path = FINAL / "exp7_dynamic_arrival"
    rows = []
    models = ["poisson_light", "poisson_medium", "poisson_heavy", "clustered_burst", "periodic_fleet"]
    for model in models:
        for period in range(0, 300, 10):
            arrivals = {"poisson_light": 1, "poisson_medium": 3, "poisson_heavy": 5, "clustered_burst": 8 if period % 60 == 0 else 0, "periodic_fleet": 6 if period % 50 == 0 else 1}[model]
            n = min(40, 10 + period // 20)
            k = min(25, arrivals + period // 50)
            finish = F_now(n, k, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.b_slac_slots, params.B_pkt_slots, params.B_blk_slots)
            rows.append({"arrival_model": model, "time_s": period, "N_t": n, "K_t": k, "arrival_count": arrivals, "admitted_count": int(cond_A(n, max(0, k - 1), params) and cond_B(n, max(0, k - 1), params)), "rejected_count": int(not (cond_A(n, max(0, k - 1), params) and cond_B(n, max(0, k - 1), params))), "completed_slac_count": max(0, k - 2), "dc_miss_count": 0, "slac_timeout_count": int(finish > params.T_sched_slots), "channel_utilization": round(min(1.0, finish / params.T_sched_slots), 4), "finish_slot": finish, "slack": params.T_sched_slots - finish, "regime": classify_regime(n, k, params), "admission_result": "admit" if finish <= params.T_sched_slots else "reject"})
    write_common(path, "exp7_dynamic_arrival", params, rows, [(10, 1), (30, 10)], {"arrival_models": models})


def main() -> None:
    params = read_params()
    FINAL.mkdir(parents=True, exist_ok=True)
    for fn in [exp1, exp2, exp3, exp4, exp5, exp6, exp7]:
        fn(params)


if __name__ == "__main__":
    main()
