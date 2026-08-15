#!/usr/bin/env python3
"""Theorem 2 completion adjudication — deterministic result generator.

Produces the committed adjudication artifacts from the exact ported
credit discipline in `theorem2_adjudication.py` (capMode 2: signed
per-session credit, aggregate allowance = q*K_active - debt,
non-preemptive straddle -> debt, persistent pointer = first blocked;
port verified against the committed e2 engine: completion cycle
indices 32/34/36/37 for K = 1/4/8/16 at q=7, PER=0, capMode 2).

Outputs (relative to the repository root):

  results/theorem2_completion.csv
      Exhaustive loss-free simultaneous-cohort sweep, q = 7, cap 0,
      all sessions admitted at cycle 0, K = 1..38, both timelines.
      Loss-free dynamics are deterministic per cohort size, so this
      sweep is a complete case analysis of the loss-free cohort
      completion claim.

  results/theorem2_adversarial.csv
      (a) The two deterministic counterexample constructions:
          staggered admission (one session per cycle, q=7, cap 0,
          K=15) and a simultaneous cohort with one session taking
          the retry cap on every message (q_wc=19, cap 2, K=35,
          within-cap demand 723 <= C_wc = 729).
      (b) An adversarial search over the loss-aware design points
          (q_wc=19/cap 2, q_wc=25/cap 3), scoped to the theorem's
          cohort premise (simultaneous admission at cycle 0; the
          necessity of that premise is established separately by the
          loss-free staggered counterexample): fixed failure
          families (all-max, victim-max/others-max at first/middle/
          last session, front/back/tail) plus a seeded random sweep
          (random.Random(7), 40 within-cap failure patterns per K,
          K in {1..16, 20, 24, 30, 35}). The search reports the
          maximum per-session elapsed cycles found; absence of a
          violation is a search result, not a proof.

  results/theorem2_counterexample_traces.txt
      Per-cycle traces (quota / allowance / consumed / carried debt /
      per-session served slots) of both counterexamples on both
      timelines, from theorem2_adjudication.run_traced().

Timelines: 'model' = the paper's 39-window convention (no credit or
service in the joining cycle); 'impl' = the committed engine
convention (credit begins in the admission cycle). Elapsed cycles are
counted from the admission cycle inclusive; the deadline is
D_g = 40T, i.e. elapsed > 40 is a violation.
"""
import csv
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from theorem2_adjudication import run, run_traced, c_actual

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'results'

TIMELINES = ('model', 'impl')
DG_CYCLES = 40


def lossfree_exhaustive():
    rows = []
    for timeline in TIMELINES:
        for k in range(1, 39):
            fails = [[0] * 20 for _ in range(k)]
            res = run(7, 0, fails, [0] * k, timeline)
            assert all(ok for _, _, ok in res)
            elapsed = [cyc for _, cyc, _ in res]
            mx = max(elapsed)
            arg = elapsed.index(mx)
            rows.append({
                'timeline': timeline, 'K': k, 'max_elapsed': mx,
                'argmax_session': arg,
                'completion_index': mx - 1,
                'violation_gt40': sum(1 for c in elapsed if c > DG_CYCLES),
            })
    return rows


def counterexamples():
    """The two deterministic constructions. Returns rows + trace text."""
    rows, traces = [], []
    cases = [
        ('staggered 1/cycle', 7, 0, 15,
         [[0] * 20 for _ in range(15)], list(range(15))),
        ('cohort victim cap-max', 19, 2, 35,
         [[2] * 20 if i == 34 else [0] * 20 for i in range(35)], [0] * 35),
    ]
    for name, q, cap, k, fails, admits in cases:
        for timeline in TIMELINES:
            res = run(q, cap, fails, admits, timeline, horizon=90)
            elapsed = [cyc for _, cyc, ok in res if ok]
            assert len(elapsed) == k, (name, timeline)
            mx = max(elapsed)
            rows.append({'pattern': name, 'q': q, 'cap': cap, 'K': k,
                         'timeline': timeline, 'max_elapsed': mx,
                         'violation_gt40': sum(1 for c in elapsed
                                               if c > DG_CYCLES)})
            done, trace = run_traced(q, cap, fails, admits, timeline,
                                     horizon=90)
            arg = max(range(k), key=lambda i: done[i] - admits[i] + 1)
            traces.append(
                f"== {name} | q={q} cap={cap} K={k} timeline={timeline} "
                f"| within-cap demand of session {arg}: "
                f"{c_actual(fails[arg])} slots | max elapsed {mx} cycles "
                f"(session {arg}, admitted cycle {admits[arg]}, "
                f"completed cycle {done[arg]}) ==")
            traces.append("cycle K_active quota allowance consumed "
                          "debt_in served(argmax)")
            for cyc, ka, quota, allw, cons, debt, served in trace:
                traces.append(f"{cyc:5d} {ka:8d} {quota:5d} {allw:9d} "
                              f"{cons:8d} {debt:7d} {served[arg]:6d}")
            traces.append("")
    return rows, "\n".join(traces) + "\n"


def adversarial_search():
    """Fixed families + seeded random sweep over the loss-aware points."""
    rng = random.Random(7)
    ks = list(range(1, 17)) + [20, 24, 30, 35]
    rows = []
    for q, cap in ((19, 2), (25, 3)):
        # random patterns are drawn once per (q,cap) and shared by both
        # timelines so the two columns search the identical space
        random_pats = {}
        for k in ks:
            pats = []
            for _ in range(40):
                fails = [[rng.randint(0, cap) for _ in range(20)]
                         for _ in range(k)]
                pats.append(fails)
            random_pats[k] = pats
        for timeline in TIMELINES:
            best, best_k, viol = 0, None, 0
            n_cfg = 0
            for k in ks:
                cand = []
                # fixed failure families x fixed offset families
                fam = [('all-max', [[cap] * 20 for _ in range(k)])]
                for victim in sorted({0, k - 1, k // 2}):
                    fam.append((f'victim-max@{victim}',
                                [[cap] * 20 if i == victim else [0] * 20
                                 for i in range(k)]))
                    fam.append((f'others-max@{victim}',
                                [[0] * 20 if i == victim else [cap] * 20
                                 for i in range(k)]))
                fam.append(('front-max', [[cap] * 10 + [0] * 10
                                          for _ in range(k)]))
                fam.append(('back-max', [[0] * 10 + [cap] * 10
                                         for _ in range(k)]))
                fam.append(('tail-max', [[0] * 17 + [cap] * 3
                                         for _ in range(k)]))
                for _, fails in fam:
                    cand.append(fails)
                cand.extend(random_pats[k])
                admits = [0] * k
                for fails in cand:
                    # enforce the within-cap demand premise C <= C_wc
                    c_env = {19: 729, 25: 970}[q]
                    if any(c_actual(f) > c_env for f in fails):
                        continue
                    n_cfg += 1
                    res = run(q, cap, fails, list(admits), timeline,
                              horizon=90)
                    for _, cyc, ok in res:
                        if not ok:
                            continue
                        if cyc > best:
                            best, best_k = cyc, k
                        if cyc > DG_CYCLES:
                            viol += 1
            rows.append({'pattern': f'adversarial sweep ({n_cfg} configs)',
                         'q': q, 'cap': cap, 'K': f'sweep(max@K={best_k})',
                         'timeline': timeline, 'max_elapsed': best,
                         'violation_gt40': viol})
    return rows


def main():
    lf = lossfree_exhaustive()
    with open(OUT / 'theorem2_completion.csv', 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(lf[0]))
        w.writeheader()
        w.writerows(lf)
    for timeline in TIMELINES:
        sub = [r for r in lf if r['timeline'] == timeline]
        mx = max(r['max_elapsed'] for r in sub)
        args = [r['K'] for r in sub if r['max_elapsed'] == mx]
        print(f"loss-free exhaustive [{timeline}]: max elapsed {mx} "
              f"cycles at K={args} | violations "
              f"{sum(r['violation_gt40'] for r in sub)}")

    ce_rows, trace_text = counterexamples()
    adv_rows = adversarial_search()
    with open(OUT / 'theorem2_adversarial.csv', 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(ce_rows[0]))
        w.writeheader()
        w.writerows(ce_rows + adv_rows)
    for r in ce_rows + adv_rows:
        print(f"{r['pattern']} [{r['timeline']}] q={r['q']}: "
              f"max elapsed {r['max_elapsed']}, "
              f"violations {r['violation_gt40']}")
    with open(OUT / 'theorem2_counterexample_traces.txt', 'w') as f:
        f.write(trace_text)


if __name__ == '__main__':
    main()
