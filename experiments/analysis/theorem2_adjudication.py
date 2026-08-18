#!/usr/bin/env python3
"""Theorem 2 adjudication checker.

Exact deterministic port of the committed credit discipline (capMode 2:
per-session signed credit, aggregate allowance = q*K_active - debt,
non-preemptive straddle -> debt, persistent pointer = first blocked).
No RNG: the adversary chooses every retry outcome (<= cap per message),
admission offsets, and session order. The checker searches for a violation
of Theorem 2's claim that an admitted session within the retry cap
completes within ceil(C_wc/q) service windows (+ joining cycle).

Timelines:
  model : the joining cycle carries no credit/service (the paper's
          39-window convention, 39 service windows before D_g).
  impl  : credit/service begin in the admission cycle (engine timeline,
          40 windows before the D_g check trips).
"""
import itertools
import sys

SEQ = [(11, 0), (11, 15), (11, 35), (11, 55), (11, 75)] + \
      [(12, 105 + 40 * i) for i in range(10)] + \
      [(18, 535), (12, 635), (12, 755), (13, 775)]
T_MS = 50.0
SUM_L = sum(l for l, _ in SEQ)
NMSG = len(SEQ)
assert SUM_L == 230 and NMSG == 19


def run(q, cap, fails, admits, timeline, horizon=60):
    """fails[i][m] = adversarial retry count (<= cap) for session i, msg m.
    admits[i] = admission cycle. Returns per-session (elapsed_windows,
    elapsed_cycles, completed) where elapsed_cycles = completed - admit + 1.
    """
    k = len(admits)
    next_m = [0] * k
    att = [0] * k
    credit = [0] * k
    done = [-1] * k
    failed = [False] * k
    remaining_fails = [list(f) for f in fails]
    g_debt = 0
    start = 0
    for cycle in range(horizon):
        active = [i for i in range(k) if done[i] < 0 and not failed[i]
                  and cycle >= (admits[i] + (1 if timeline == 'model' else 0))]
        if not active:
            if all(d >= 0 or failed[i] for i, d in enumerate(done)):
                break
            continue
        quota = q * len(active)
        for i in active:
            credit[i] += q
        allowance = quota - g_debt
        consumed = 0
        first_blocked = -1
        order = [(start + j) % k for j in range(k)]
        for idx in order:
            if idx not in active:
                continue
            i = idx
            while credit[i] > 0 and next_m[i] < NMSG:
                length, rel = SEQ[next_m[i]]
                if (cycle - admits[i]) * T_MS < rel:
                    break
                if consumed >= allowance:
                    if first_blocked < 0:
                        first_blocked = i
                    break
                credit[i] -= length
                consumed += length
                if remaining_fails[i][next_m[i]] > 0:
                    remaining_fails[i][next_m[i]] -= 1
                    att[i] += 1
                    if cap > 0 and att[i] > cap:
                        failed[i] = True
                        break
                else:
                    next_m[i] += 1
                    att[i] = 0
                    if next_m[i] == NMSG:
                        done[i] = cycle
                        break
        g_debt = max(0, g_debt + consumed - quota)
        start = first_blocked if first_blocked >= 0 else 0
    out = []
    for i in range(k):
        if done[i] < 0:
            out.append((None, None, False))
        else:
            first_window = admits[i] + (1 if timeline == 'model' else 0)
            out.append((done[i] - first_window + 1, done[i] - admits[i] + 1,
                        True))
    return out


def c_actual(fails_i):
    return sum(l * (1 + f) for (l, _), f in zip(SEQ, fails_i))


def check(q, cap, fails, admits, timeline):
    """Return list of violation dicts."""
    res = run(q, cap, fails, admits, timeline)
    viol = []
    for i, (win, cyc, ok) in enumerate(res):
        cw = c_actual(fails[i])
        c_env = {7: 247, 19: 707, 25: 937}[q]   # theorem's C_wc envelope
        bound_win = -(-c_env // q)       # ceil(C_wc/q) service windows
        dg_cycles = 40
        if not ok:
            viol.append({'i': i, 'why': 'never completed', 'q': q,
                         'admits': admits, 'fails': fails, 'tl': timeline})
            continue
        if win > bound_win:
            viol.append({'i': i, 'why': f'windows {win} > ceil(C/q)={bound_win}',
                         'C': cw, 'q': q, 'admits': admits,
                         'failsum': [sum(f) for f in fails], 'tl': timeline})
        if cyc > dg_cycles:
            viol.append({'i': i, 'why': f'elapsed {cyc} > D_g cycles 40',
                         'C': cw, 'q': q, 'admits': admits,
                         'failsum': [sum(f) for f in fails], 'tl': timeline})
    return viol


def main():
    settings = [(7, 0), (19, 2), (25, 3)]
    windows_viol, dg_viol = [], []
    n_cfg = 0

    def fail_patterns(cap, k, victim):
        pats = []
        maxf = [ [cap] * NMSG for _ in range(k)]
        pats.append(('all-max', maxf))
        if cap > 0:
            v_only = [[cap] * NMSG if i == victim else [0] * NMSG for i in range(k)]
            pats.append(('victim-max', v_only))
            others = [[0] * NMSG if i == victim else [cap] * NMSG for i in range(k)]
            pats.append(('others-max', others))
            front = [[cap] * (NMSG // 2) + [0] * (NMSG - NMSG // 2) for _ in range(k)]
            pats.append(('front-max', front))
            back = [[0] * (NMSG - NMSG // 2) + [cap] * (NMSG // 2) for _ in range(k)]
            pats.append(('back-max', back))
            last3 = [[0] * (NMSG - 3) + [cap] * 3 for _ in range(k)]
            pats.append(('tail-max', last3))
        else:
            pats.append(('none', [[0] * NMSG for _ in range(k)]))
        return pats

    for timeline in globals().get('_TIMELINES', ('model', 'impl')):
        for q, cap in settings:
            for k in list(range(1, 9)) + [12, 16, 24, 35]:
                # admission offset families
                offset_sets = [[0] * k]
                for d in (1, 2, 3, 5, 10, 20):
                    offset_sets.append([0] * (k - 1) + [d])       # one late
                    offset_sets.append([d] + [0] * (k - 1))       # one early
                    offset_sets.append([j % d if d else 0 for j in range(k)])
                if k <= 4:
                    offset_sets += [list(t) for t in
                                    itertools.product((0, 1, 3, 7), repeat=k)]
                seen = set()
                for admits in offset_sets:
                    key = tuple(admits)
                    if key in seen:
                        continue
                    seen.add(key)
                    for victim in ({0, k - 1, k // 2} if k > 1 else {0}):
                        for name, fails in fail_patterns(cap, k, victim):
                            n_cfg += 1
                            for v in check(q, cap, fails, list(admits), timeline):
                                v['pattern'] = name
                                if 'windows' in v['why']:
                                    windows_viol.append(v)
                                else:
                                    dg_viol.append(v)
    print(f"configurations checked: {n_cfg}")
    print(f"window-bound violations (win > ceil(C/q)): {len(windows_viol)}")
    for v in windows_viol[:8]:
        print("  ", v)
    print(f"D_g violations (elapsed > 40): {len(dg_viol)}")
    for v in dg_viol[:8]:
        print("  ", v)
    if not windows_viol and not dg_viol:
        print("NO VIOLATION FOUND in the adversarial search space.")


def run_traced(q, cap, fails, admits, timeline, horizon=90):
    """run() with a per-cycle (active, quota, allowance, consumed, debt,
    per-session served) trace, for timeline sanity diffs. Semantics identical
    to run()."""
    k = len(admits)
    next_m = [0] * k
    att = [0] * k
    credit = [0] * k
    done = [-1] * k
    failed = [False] * k
    remaining_fails = [list(f) for f in fails]
    g_debt = 0
    start = 0
    trace = []
    served = [0] * k
    for cycle in range(horizon):
        active = [i for i in range(k) if done[i] < 0 and not failed[i]
                  and cycle >= (admits[i] + (1 if timeline == 'model' else 0))]
        if not active:
            if all(d >= 0 or failed[i] for i, d in enumerate(done)):
                break
            trace.append((cycle, 0, 0, 0, 0, g_debt, tuple(served)))
            continue
        quota = q * len(active)
        for i in active:
            credit[i] += q
        allowance = quota - g_debt
        consumed = 0
        first_blocked = -1
        order = [(start + j) % k for j in range(k)]
        for idx in order:
            if idx not in active:
                continue
            i = idx
            while credit[i] > 0 and next_m[i] < NMSG:
                length, rel = SEQ[next_m[i]]
                if (cycle - admits[i]) * T_MS < rel:
                    break
                if consumed >= allowance:
                    if first_blocked < 0:
                        first_blocked = i
                    break
                credit[i] -= length
                consumed += length
                served[i] += length
                if remaining_fails[i][next_m[i]] > 0:
                    remaining_fails[i][next_m[i]] -= 1
                    att[i] += 1
                    if cap > 0 and att[i] > cap:
                        failed[i] = True
                        break
                else:
                    next_m[i] += 1
                    att[i] = 0
                    if next_m[i] == NMSG:
                        done[i] = cycle
                        break
        trace.append((cycle, len(active), quota, allowance, consumed, g_debt,
                      tuple(served)))
        g_debt = max(0, g_debt + consumed - quota)
        start = first_blocked if first_blocked >= 0 else 0
    return done, trace


if __name__ == '__main__':
    # --timeline {impl,model}: restricts main()'s search to one timeline.
    # Default (no flag) searches both timelines.
    # 'model' semantics: a session accrues no credit in its admission
    # cycle, is not served in it, and is excluded from the aggregate
    # quota for it; message releases stay ABSOLUTE from admission.
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--timeline', choices=('impl', 'model'), default=None)
    args = ap.parse_args()
    if args.timeline is None:
        main()
    else:
        _orig = ('model', 'impl')
        import builtins
        # narrow main()'s timeline loop without altering its definition
        globals()['_TIMELINES'] = (args.timeline,)
        main()
