# gap-aware-plc-admission_public

> Submission snapshot for ICIT 2027 — tag `icit27-submission-v1`.
> This is the artifact repository cited by the paper's Sec. V footnote.

Artifact for the ICIT 2027 submission.

This repository contains the reviewer-facing simulation and evidence package
corresponding to the submitted paper.

Start here:
[`docs/PARAMETER_PROVENANCE.md`](docs/PARAMETER_PROVENANCE.md),
[`results/RESULTS.md`](results/RESULTS.md).

> The event-driven tier is a custom frame/slot-level error-channel model, not a HomePlug OFDM PHY implementation.
> The per-link attenuation and Gilbert–Elliott settings are controlled
> evaluation inputs and stress parameters, not calibrated point estimates
> for a specific charging site.

Simulation and evidence package for gap-aware admission control of EV
charging communication on a shared HPGP power-line channel: SLAC
authentication completion creates future periodic DC load, so admission
must check both the current state and the post-transition future state.

Paper: *Deterministic Multi-EV Charging over Shared PLC: Gap-Aware
Admission Control for Mixed Authentication and Control Traffic*,
Dawoon Kim and Chang-Gun Lee (Seoul National University).

## Layout

| Path | Contents |
|---|---|
| `src/` | Single source-of-truth formulas (`src/formulas/transition_formulas.py`) and the Layer-1 slot builder (`src/sim/cycle_builder.py`) |
| `ns3/contrib/ev-plc-transition/` | ns-3 contrib module: grant-map scheduler, slot machine, event-driven policy MAC, PLC channel (per-link attenuation, Gilbert–Elliott bursty noise), SLAC session model |
| `ns3/standalone/` | g++ harness that compiles the same module sources against stub headers (`build.sh`); produces the slot-accurate and event-approximation binaries |
| `experiments/` | Runners (`run/`, `ns3_e1/`, `layer1/`), configs and seed lists (`configs/`), analysis/plot helpers (`analysis/`), expected outputs (`expected/`) |
| `results/` | Committed outputs the paper's numbers come from (see `results/RESULTS.md`) |
| `figures/` | Paper figure generation scripts |
| `tests/` | Invariant tests (Python 18, C++ suites via `run_cpp_tests.sh`) |
| `tools/` | `setup_ns3.sh`, module sync, structure check, gate scripts |

## Environment

- Python 3.12: `tools/setup_python_env.sh` (builds matplotlib against the
  system FreeType — required for the figure bounding-box gate)
- ns-3: upstream `ns-3-dev` pinned at commit `033dc84a` — run
  `tools/setup_ns3.sh [--build]` (clones as a sibling directory and
  symlinks `ns3/contrib/ev-plc-transition` into it). The pin is a fixed
  upstream master commit, not a tagged release; cite it as
  `ns-3-dev @033dc84a`.

Three execution tiers, in increasing fidelity: Layer-1 Python slot builder;
slot-accurate simulator (ns-3 contrib module, bit-identical between the
standalone and real ns-3 builds); event-driven ns-3 with the PLC channel
model.

## Aggregate window cap (service discipline)

Per-cycle authentication occupancy is capped at the aggregate window
qK with a carried debt (`debt = max(0, debt + consumed - quota)`); a
persistent service pointer resumes from the first blocked session, so
no session starves, and the assertion `consumed <= ceil(quota) + 17`
is active in every capped run. Implemented in `ev-plc-policy-mac`,
`e2_sim`, and `e4_sim` (`loss_sim` already used the aggregate-debt
discipline). This cap structurally guarantees the paper's Lemma 1
(realized window occupancy <= qK + B_str); see `results/RESULTS.md`.

## Reproduction

- Full test gate: `bash tests/run_cpp_tests.sh` (standalone + ns-3 suites)
  and `for t in tests/test_*.py; do python3 "$t"; done`
- Layer-1 knee verification: `python3 experiments/layer1/verify_knee_e1.py`
- Admission region: `python3 experiments/layer1/sweep_admission_region.py`
- Event-driven policy comparison (Fig. 4 data):
  `python3 experiments/ns3_e1/run_trackc_c4.py`
- Parity gates: `python3 experiments/ns3_e1/run_trackc_g1.py`

## Which script makes which figure/table

The gap-aware policy appears as `acbs` in legacy code identifiers and
CSV policy keys (e.g. the `acbs_qwc25` row of the policy-comparison
CSVs); the identifier is historical and carries no methodological
meaning. This repository holds the simulation code, experiment configurations,
committed result CSVs and selected traces, and the generators for the paper's
result figures (Figs. 2-4) and tables; the concept figure (Fig. 1) is maintained in the paper
repository. Figure script filenames keep their historical numbering: `make_fig5_policy.py`
renders the paper's Fig. 4, and `make_fig4_knee.py` renders a
supplementary figure of the Table I knee data (not shipped in the paper).

| Paper item | Data | Script | Policy key |
|---|---|---|---|
| Fig. 2 (admission-envelope cycle) | Layer-1 | `figures/make_fig2_cycle.py` | — |
| Fig. 3 (feasibility regimes) | `results/layer1/admission_region.csv` | `figures/make_fig3_region.py` | — |
| Fig. 4 (policy comparison) | `results/ns3_e1/e4_policy_comparison_event.csv` | `figures/make_fig5_policy.py` | `acbs_qwc25` = gap-aware; `hpgp_csma_ca`, `fixed_10pct/25pct/50pct` |
| Table I (admission knees vs IFS) | `results/layer1/knee_verification.csv` (IFS=0), `results/ns3_e1/overhead_knee.csv` (IFS 1–3) | `experiments/layer1/verify_knee_e1.py`, `experiments/ns3_e1/run_overhead.py` | — |
| Table II (bursty vs i.i.d. by residual room) | `results/ns3_e1/g3_burst_vs_iid.csv`, `results/ns3_e1/g3i_iid_baseline.csv` | `experiments/ns3_e1/run_g3_burst_vs_iid.py` | — |
| Sec. V-C loss outcomes | `results/ns3_e1/e2_admission_variants.csv` | `experiments/ns3_e1/run_e2.py` | variants `A_loss_blind` (q=7), `C_q_wc` (q_wc caps) |

Numeric claims and their producing runs are indexed in
`results/RESULTS.md`.

Note: `tools/check_figures.sh` verifies figure content in its default mode;
its `--strict` bounding-box comparison is an internal typesetting gate, not
something external reproduction needs — it only holds when matplotlib is
built against the system FreeType (`tools/setup_python_env.sh`, optional).

## License

MIT — see `LICENSE`.
