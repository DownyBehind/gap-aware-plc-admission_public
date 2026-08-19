#!/usr/bin/env python3
"""ISO 15118-2 CurrentDemand airtime provenance check (Phase 13).

Reproduces the paper's byte-to-slot conversion for the encoded EXI
reference profiles recorded in
results/provenance/iso15118_currentdemand_sizes.csv and checks them
against the published analysis bounds C_req = 15 and C_res = 21 slots.

    slots = ceil((t_fixed + 8 * (L_EXI + L_stack) / R) / DELTA)

with L_stack = 82 B (Eth 14 + IPv6 40 + TCP 20 + V2GTP 8),
t_fixed = 350 us, DELTA = 35.84 us, at both R = 10 Mbps (the paper's
reference profile -- this rate decides PASS/FAIL) and R = 9.8452 Mbps
(the HPGP-specified HS-ROBO rate, reported as sensitivity).

This script does not encode anything; the EXI byte counts are produced
by experiments/provenance/encode_currentdemand.c (EVerest libcbv2g)
and are fixed inputs here. The check validates that the encoded
reference frames remain within the analysis bounds; it is not a claim
about universal protocol maxima.
"""
import csv
import math
import os

DELTA_US = 35.84
T_FIXED_US = 350.0
L_STACK = 82
RATES = {"10Mbps": 10e6, "9.8452Mbps": 9.8452e6}
BOUNDS = {"CurrentDemandReq": 15, "CurrentDemandRes": 21}

HERE = os.path.dirname(os.path.abspath(__file__))
CSV = os.path.join(HERE, "..", "..", "results", "provenance",
                   "iso15118_currentdemand_sizes.csv")


def slots(exi_bytes: int, rate_bps: float) -> tuple[int, float]:
    airtime_us = T_FIXED_US + 8.0 * (exi_bytes + L_STACK) / rate_bps * 1e6
    return math.ceil(airtime_us / DELTA_US), airtime_us


def max_exi_for(bound_slots: int, rate_bps: float) -> int:
    """Largest L_EXI (bytes) with slots(L_EXI) <= bound_slots."""
    budget_us = bound_slots * DELTA_US - T_FIXED_US
    return math.floor(budget_us * rate_bps / 8.0 / 1e6) - L_STACK


def main() -> None:
    with open(CSV, newline="") as f:
        rows = list(csv.DictReader(f))

    print("== pass-line reproduction (max admissible L_EXI per bound) ==")
    for rname, r in RATES.items():
        print(f"  R = {rname}: C_req=15 -> L_EXI <= {max_exi_for(15, r)} B, "
              f"C_res=21 -> L_EXI <= {max_exi_for(21, r)} B")

    print("\n== per-profile check ==")
    print(f"{'message':<18} {'profile':<13} {'EXI B':>5} {'total B':>7} "
          f"{'us@10':>8} {'slots@10':>8} {'us@9.8452':>10} {'slots@9.8452':>12} "
          f"{'bound':>5}  verdict")
    all_pass_ref = True
    for row in rows:
        msg, prof, n = row["message"], row["profile"], int(row["exi_bytes"])
        bound = BOUNDS[msg]
        s10, a10 = slots(n, RATES["10Mbps"])
        s98, a98 = slots(n, RATES["9.8452Mbps"])
        ok10 = s10 <= bound
        ok98 = s98 <= bound
        all_pass_ref &= ok10
        verdict = (f"{'PASS' if ok10 else 'FAIL'}@10Mbps / "
                   f"{'PASS' if ok98 else 'FAIL'}@9.8452Mbps")
        print(f"{msg:<18} {prof:<13} {n:>5} {n + L_STACK:>7} "
              f"{a10:>8.1f} {s10:>8} {a98:>10.1f} {s98:>12} {bound:>5}  {verdict}")

    # The reference profile (R = 10 Mbps) decides the check.
    for row in rows:
        s10, _ = slots(int(row["exi_bytes"]), RATES["10Mbps"])
        assert s10 <= BOUNDS[row["message"]], (
            f"{row['message']}/{row['profile']}: {s10} slots exceeds "
            f"bound {BOUNDS[row['message']]} at R = 10 Mbps")
    print("\nassert OK: all encoded reference profiles fit the published "
          "bounds at R = 10 Mbps"
          + ("" if all_pass_ref else "  (unreachable)"))


if __name__ == "__main__":
    main()
