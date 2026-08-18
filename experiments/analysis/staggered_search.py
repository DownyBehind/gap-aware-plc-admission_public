#!/usr/bin/env python3
"""Pre-declared exhaustive staggered-admission search for Theorem 2.

Complements the simultaneous-cohort sweep and the adversarial search of
run_theorem2_adjudication.py with a deterministic, pre-declared search
over staggered admission patterns. Search space (fixed; no RNG):

  K        : 2..38
  stagger  : period p in {1, 2, 3, 5} cycles, admits[i] = i * p
  setting  : (q, cap) in {(7, 0), (19, 2), (25, 3)}
  timeline : {model, impl}
  patterns : cap == 0 -> 'none'; cap > 0 -> all-max, victim-max /
             others-max at victim in {0, K//2, K-1} (deduplicated),
             front-max ([cap]*9 + [0]*10), back-max ([0]*10 + [cap]*9),
             tail-max ([0]*16 + [cap]*3)
  filter   : within-cap iff max_i c_actual(fails_i) <= C_wc,
             C_wc = {7: 247, 19: 707, 25: 937}
  horizon  : 400 cycles (large enough that every admitted session
             completes; the latest admission is cycle 185)

Calls theorem2_adjudication.run() unmodified. Rows with
within_cap == 0 are recorded in the CSV but excluded from violation
adjudication (violation_gt40 is reported as 0 for them).

Output: results/theorem2_staggered.csv (6,184 rows).
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from theorem2_adjudication import run, c_actual, SEQ, SUM_L, NMSG

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'results'

C_WC = {7: 247, 19: 707, 25: 937}
HORIZON = 400
DG = 40
KS = range(2, 39)
PERIODS = (1, 2, 3, 5)
SETTINGS = ((7, 0), (19, 2), (25, 3))
TIMELINES = ('model', 'impl')


def patterns(cap, k):
    """Yield (name, victim, fails). Same family construction as
    run_theorem2_adjudication.adversarial_search()."""
    if cap == 0:
        yield ('none', None, [[0] * NMSG for _ in range(k)])
        return
    yield ('all-max', None, [[cap] * NMSG for _ in range(k)])
    for v in sorted({0, k // 2, k - 1}):
        yield ('victim-max', v,
               [[cap] * NMSG if i == v else [0] * NMSG for i in range(k)])
        yield ('others-max', v,
               [[0] * NMSG if i == v else [cap] * NMSG for i in range(k)])
    yield ('front-max', None,
           [[cap] * (NMSG // 2) + [0] * (NMSG - NMSG // 2)
            for _ in range(k)])
    yield ('back-max', None,
           [[0] * (NMSG - NMSG // 2) + [cap] * (NMSG // 2)
            for _ in range(k)])
    yield ('tail-max', None, [[0] * (NMSG - 3) + [cap] * 3
                              for _ in range(k)])


def main():
    rows, incomplete = [], []
    n_cfg = n_within = 0
    viol_total = 0
    excluded_raw_gt40 = 0
    for timeline in TIMELINES:
        for q, cap in SETTINGS:
            for k in KS:
                for p in PERIODS:
                    admits = [i * p for i in range(k)]
                    for name, victim, fails in patterns(cap, k):
                        n_cfg += 1
                        cmax = max(c_actual(f) for f in fails)
                        within = 1 if cmax <= C_WC[q] else 0
                        n_within += within
                        res = run(q, cap, fails, admits, timeline,
                                  horizon=HORIZON)
                        elapsed = [cyc for _, cyc, ok in res if ok]
                        n_done = len(elapsed)
                        if n_done < k:
                            incomplete.append((timeline, q, cap, k, p,
                                               name, victim, within,
                                               k - n_done))
                        if elapsed:
                            mx = max(elapsed)
                            arg = next(i for i, (_, cyc, ok)
                                       in enumerate(res)
                                       if ok and cyc == mx)
                        else:
                            mx, arg = -1, -1
                        raw_gt40 = sum(1 for c in elapsed if c > DG)
                        viol = raw_gt40 if within else 0
                        if within:
                            viol_total += viol
                        else:
                            excluded_raw_gt40 += raw_gt40
                        rows.append({
                            'timeline': timeline, 'q': q, 'cap': cap,
                            'K': k, 'period': p, 'pattern': name,
                            'victim': '' if victim is None else victim,
                            'max_elapsed': mx, 'argmax_session': arg,
                            'c_actual': cmax, 'within_cap': within,
                            'violation_gt40': viol,
                        })
    path = OUT / 'theorem2_staggered.csv'
    with open(path, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    print(f'sequence: {NMSG} messages, sum {SUM_L} slots')
    print(f'configs: {n_cfg} | within-cap: {n_within}')
    print(f'adjudicated violation_gt40 total (within-cap only): '
          f'{viol_total}')
    print(f'raw >40 counts on excluded (within_cap=0) rows: '
          f'{excluded_raw_gt40}')
    print(f'incomplete-in-horizon configs: {len(incomplete)}')
    for t in incomplete[:20]:
        print('  INCOMPLETE', t)
    print(f'csv: {path}')


if __name__ == '__main__':
    main()
