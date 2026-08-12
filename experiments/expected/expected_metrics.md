# Expected Metrics

- Exp 1: scheduled-after-admission should reduce DC response tails and deadline misses relative to contention-only.
- Exp 2: adaptive authentication credit should reduce idle reserved capacity without increasing SLAC timeouts under admitted bursts.
- Exp 3: Cond-A-only should show over-admission near `N=37,K=1`; Cond-A+B should prevent the unsafe admission.
- Exp 4: hidden, paid, and rejected regions should appear in the N-K plane; paid-regime slack should degrade by `C_req_eff + C_res_eff - b_auth`.
