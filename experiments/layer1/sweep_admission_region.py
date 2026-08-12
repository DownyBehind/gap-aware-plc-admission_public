#!/usr/bin/env python3
"""Layer-1 design-space sweep: (N, K) admission region heatmap (Stage 3, §5.1).

Cross-validates the analytic classification (classify_regime) against an
independent reconstruction from cycle-builder geometry on every grid cell,
then renders the figure-first candidate: the four-regime admission region
with the hidden/paid boundary q*K + B_pkt = G(N), the admission boundary
F_DC(N+K) = T_sched, and the gap-closure line N* = 20.
"""

from pathlib import Path
import csv
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import BoundaryNorm, ListedColormap
from matplotlib.patches import Patch

sys.path.insert(0, str(ROOT / "experiments" / "analysis"))
from ieee_plot_style import apply_ieee_style

from src.formulas.transition_formulas import TransitionParams, classify_regime
from src.sim.cycle_builder import build_cycle

N_MAX, K_MAX = 45, 20
REGIMES = ["setup_only", "hidden", "paid", "rejected"]
# Luminance-ordered region fills (grayscale-print monotone 0.91 -> 0.36);
# CVD-validated (worst adjacent dE 16.5), relieved by direct region labels.
REGIME_COLORS = {"setup_only": "#e0e0e0", "hidden": "#9ecae1", "paid": "#4292c6", "rejected": "#0a3a75"}

OUT_DIR = ROOT / "results" / "layer1"


def regime_from_builder(n: int, k: int, params: TransitionParams) -> str:
    """Reconstruct the regime from cycle-builder geometry only (no classify_regime)."""
    if n == 0:
        return "setup_only"
    cycle = build_cycle(n, k, params)
    post_transition = build_cycle(n + k, 0, params)
    if cycle.finish > params.T_sched_slots or post_transition.finish > params.T_sched_slots:
        return "rejected"
    return "hidden" if cycle.overflow_len == 0 else "paid"


def sweep(params: TransitionParams) -> list[dict]:
    rows = []
    for n in range(0, N_MAX + 1):
        for k in range(0, K_MAX + 1):
            formula = classify_regime(n, k, params)
            builder = regime_from_builder(n, k, params)
            assert formula == builder, f"regime mismatch at (N={n}, K={k}): formula={formula}, builder={builder}"
            cycle = build_cycle(n, k, params)
            rows.append({
                "N": n,
                "K": k,
                "regime": formula,
                "finish": cycle.finish,
                "slack": params.T_sched_slots - cycle.finish,
                "auth_span": cycle.auth_span,
                "hidden_len": cycle.hidden_len,
                "overflow_len": cycle.overflow_len,
            })
    return rows


def check_diagonal_boundary(rows: list[dict]) -> dict[int, int]:
    """Completion criterion (b): hidden and paid both exist for every K > 0 and
    the hidden->paid transition N is non-increasing in K (diagonal boundary)."""
    transition_n = {}
    by_k = {}
    for r in rows:
        by_k.setdefault(r["K"], {})[r["N"]] = r["regime"]
    for k in range(1, K_MAX + 1):
        regimes = by_k[k]
        hidden_ns = [n for n, reg in regimes.items() if reg == "hidden"]
        paid_ns = [n for n, reg in regimes.items() if reg == "paid"]
        assert hidden_ns, f"K={k}: no hidden cell"
        assert paid_ns, f"K={k}: no paid cell"
        transition_n[k] = min(paid_ns)
    knees = [transition_n[k] for k in range(1, K_MAX + 1)]
    assert all(a >= b for a, b in zip(knees, knees[1:])), f"transition N not non-increasing: {knees}"
    return transition_n


def spot_checks(params: TransitionParams) -> dict[str, str]:
    """Completion criterion (c)."""
    from src.formulas.transition_formulas import F_DC, cond_B

    checks = {}
    assert classify_regime(37, 1, params) == "paid"
    checks["(37,1)"] = "paid (admitted state)"
    assert classify_regime(36, 1, params) == "paid"
    assert cond_B(36, 1, params) is True
    checks["(36,1)"] = "paid, Cond B admit (F_DC(38) = 1389 <= 1395)"
    # Rejected boundary starts where the post-transition state N+K = 39 first
    # exceeds T_sched, matching Cond B which checks N+K+1.
    assert classify_regime(38, 1, params) == "rejected"
    assert classify_regime(37, 2, params) == "rejected"
    assert F_DC(39, params.C_req_slots, params.C_res_slots, params.C_proc_slots, params.B_blk_slots) > params.T_sched_slots
    assert cond_B(37, 1, params) is False
    checks["rejected boundary"] = "starts at N+K = 39 (F_DC(39) = 1425 > 1395), consistent with Cond B(37,1) = False"
    return checks


def render(rows: list[dict], params: TransitionParams) -> list[Path]:
    apply_ieee_style()
    index = {r: i for i, r in enumerate(REGIMES)}
    grid = [[0] * (N_MAX + 1) for _ in range(K_MAX + 1)]
    for r in rows:
        grid[r["K"]][r["N"]] = index[r["regime"]]

    # height 2.16 fits the paper page budget; axis labels, legend, and
    # knee-marker sizes are sized for that height.
    fig, ax = plt.subplots(figsize=(3.5, 2.16))
    cmap = ListedColormap([REGIME_COLORS[r] for r in REGIMES])
    norm = BoundaryNorm([-0.5, 0.5, 1.5, 2.5, 3.5], cmap.N)
    xs = [n - 0.5 for n in range(0, N_MAX + 2)]
    ys = [k - 0.5 for k in range(0, K_MAX + 2)]
    ax.pcolormesh(xs, ys, grid, cmap=cmap, norm=norm, rasterized=False)

    # Hidden/paid boundary: q*K + B_pkt = G(N), continuous, K > 0 region only.
    q, b_pkt = params.b_slac_slots, params.B_pkt_slots
    bn, bk = [], []
    n = 1.0
    while n <= 20.0:
        g = max(0.0, params.C_proc_slots - (n - 1.0) * params.C_req_slots)
        k_b = (g - b_pkt) / q
        if 0.0 < k_b <= K_MAX + 0.5:
            bn.append(n)
            bk.append(k_b)
        n += 0.05
    ax.plot(bn, bk, color="black", linewidth=1.0, label=r"$qK + B_{\mathrm{pkt}} = G(N)$")

    # E1 knee cells (cross-reference with fig_knee_delta_vs_N).
    knee_cells = [(18, 1), (17, 4), (15, 8), (11, 16)]
    ax.plot([n for n, _ in knee_cells], [k for _, k in knee_cells], "o",
            color="black", markersize=3, markerfacecolor="white",
            markeredgewidth=0.8, linestyle="none", label="admission knees", zorder=5)

    # Admission boundary: F_DC(N+K) = T_sched  =>  N + K = 1374/36.
    m_star = (params.T_sched_slots - params.B_blk_slots) / (params.C_req_slots + params.C_res_slots)
    ax.plot([m_star - K_MAX, m_star], [K_MAX, 0], color="black", linestyle="--", linewidth=1.0, label=r"$F_{\mathrm{DC}}(N{+}K) = T$")

    # Gap closure: first N with G(N) = 0.
    ax.axvline(20, color="black", linestyle=":", linewidth=0.8)
    ax.annotate("gap closure ($G(N){=}0$)", xy=(20.6, 1.5), fontsize=6, ha="left", color="white")

    # Direct region labels (relief for low-contrast fills).
    ax.text(6, 3.5, "hidden", fontsize=7, ha="center")
    ax.text(27, 6, "paid", fontsize=7, ha="center", color="white")
    ax.text(41, 4, "rejected", fontsize=7, ha="center", color="white")

    ax.set_xlabel("DC-active EVs, $N$")
    ax.set_ylabel("Active SLAC sessions, $K$")
    ax.yaxis.set_major_locator(plt.MaxNLocator(integer=True))
    ax.set_xlim(-0.5, N_MAX + 0.5)
    ax.set_ylim(-0.5, K_MAX + 0.5)
    handles = [Patch(facecolor=REGIME_COLORS[r], label=r.replace("_", "-")) for r in REGIMES]
    handles += ax.get_legend_handles_labels()[0]
    ax.legend(handles=handles, loc="upper right", fontsize=5.5, framealpha=0.95, handlelength=1.2, borderpad=0.4)
    fig.tight_layout(pad=0.3)

    outputs = []
    for ext in ("pdf", "png"):
        out = OUT_DIR / f"fig_admission_region.{ext}"
        fig.savefig(out, bbox_inches="tight")
        outputs.append(out)
    plt.close(fig)
    return outputs


def main() -> None:
    params = TransitionParams()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    rows = sweep(params)  # criterion (a): asserts every cell
    assert len(rows) == (N_MAX + 1) * (K_MAX + 1) == 966

    transition_n = check_diagonal_boundary(rows)  # criterion (b)
    checks = spot_checks(params)  # criterion (c)

    csv_path = OUT_DIR / "admission_region.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    outputs = render(rows, params)  # criterion (d)
    for out in outputs:
        assert out.exists() and out.stat().st_size > 0

    print(f"cells: {len(rows)} (regime cross-validation: all match)")
    print(f"csv:   {csv_path}")
    for out in outputs:
        print(f"fig:   {out}")
    print("hidden->paid transition N by K:", {k: transition_n[k] for k in [1, 4, 8, 16, 20]})
    for name, result in checks.items():
        print(f"spot check {name}: {result}")


if __name__ == "__main__":
    main()
