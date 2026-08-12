#!/usr/bin/env python3
import csv
import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

REGIME_VALUE = {'hidden': 0, 'paid': 1, 'rejected': 2}
REGIME_LABEL = {0: 'hidden', 1: 'paid', 2: 'rejected'}


def read_rows(path):
    with (Path(path) / 'metrics.csv').open(newline='') as f:
        return list(csv.DictReader(f))


def save(out, stem):
    out.mkdir(parents=True, exist_ok=True)
    plt.tight_layout()
    plt.savefig(out / f'{stem}.png', dpi=300)
    plt.savefig(out / f'{stem}.pdf')
    plt.close()


def main():
    result_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('results/exp4_three_regime')
    rows = read_rows(result_dir)
    out = result_dir / 'figures'

    ns = sorted({int(r['N']) for r in rows})
    ks = sorted({int(r['K']) for r in rows})
    n_index = {n: i for i, n in enumerate(ns)}
    k_index = {k: i for i, k in enumerate(ks)}
    grid = [[float('nan') for _ in ns] for _ in ks]
    slack_points = []
    for r in rows:
        n = int(r['N']); k = int(r['K'])
        regime = r['regime'].lower()
        grid[k_index[k]][n_index[n]] = REGIME_VALUE.get(regime, float('nan'))
        if regime == 'paid':
            slack_points.append((n, k, float(r['slack_slots'])))

    plt.figure(figsize=(7.2, 4.8))
    image = plt.imshow(grid, origin='lower', aspect='auto', extent=[min(ns), max(ns), min(ks), max(ks)], vmin=0, vmax=2)
    cbar = plt.colorbar(image, ticks=[0, 1, 2])
    cbar.ax.set_yticklabels([REGIME_LABEL[i] for i in [0, 1, 2]])
    plt.xlabel('N active DC sessions')
    plt.ylabel('K active SLAC sessions')
    plt.title('Exp4 three-regime map')
    save(out, 'exp4_three_regime_heatmap')

    slack_points.sort(key=lambda x: (x[0], x[1]))
    plt.figure(figsize=(7.2, 4.0))
    if slack_points:
        x = list(range(len(slack_points)))
        y = [p[2] for p in slack_points]
        plt.plot(x, y, linewidth=1.2)
    plt.xlabel('Paid-regime sample index')
    plt.ylabel('Slack slots')
    plt.title('Exp4 paid-regime slack degradation')
    plt.grid(True, alpha=0.3)
    save(out, 'exp4_slack_degradation')

    plt.figure(figsize=(6.0, 3.8))
    y = [float(r.get('slack_slots') or 0) for r in rows]
    plt.plot(list(range(len(y))), y, marker='.', linewidth=0.8)
    plt.title('Exp4 slack samples')
    plt.xlabel('sample')
    plt.ylabel('slack slots')
    plt.grid(True, alpha=0.3)
    save(out, 'exp4_plot')

if __name__ == '__main__':
    main()
