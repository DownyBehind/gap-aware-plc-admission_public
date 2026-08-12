#!/usr/bin/env python3
import csv
import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def read_csv(path):
    with Path(path).open(newline='') as f:
        return list(csv.DictReader(f))


def save_both(out, stem):
    out.mkdir(parents=True, exist_ok=True)
    plt.tight_layout()
    plt.savefig(out / f'{stem}.png', dpi=300)
    plt.savefig(out / f'{stem}.pdf')
    plt.close()


def f(row, key):
    return float(row[key])


def main():
    result_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('results/exp2_fixed_vs_adaptive')
    out = result_dir / 'figures'
    fixed = read_csv(result_dir / 'fixed_by_bfix_summary.csv')
    adaptive = read_csv(result_dir / 'adaptive_summary.csv')[0]

    bfix = [int(float(r['B_fix'])) for r in fixed]
    idle = [f(r, 'mean_idle_waste_per_period') for r in fixed]
    timeout = [f(r, 'slac_timeout_ratio') for r in fixed]
    adaptive_idle = f(adaptive, 'total_idle_waste')
    adaptive_timeout = (f(adaptive, 'timed_out_slac_sessions') / f(adaptive, 'admitted_slac_sessions')) if f(adaptive, 'admitted_slac_sessions') else 0.0

    plt.figure(figsize=(6.2, 4.0))
    plt.scatter(idle, timeout, s=70, label='fixed reservation family')
    for x, y, label in zip(idle, timeout, bfix):
        plt.annotate(f'B={label}', (x, y), textcoords='offset points', xytext=(5, 5), fontsize=9)
    plt.scatter([adaptive_idle], [adaptive_timeout], marker='*', s=160, label='adaptive transition-aware')
    plt.xlabel('Mean idle reserved slots per period')
    plt.ylabel('SLAC timeout ratio among admitted sessions')
    plt.title('Fixed reservation trade-off')
    plt.grid(True, alpha=0.3)
    plt.legend(frameon=False)
    save_both(out, 'fixed_tradeoff_idle_vs_timeout')

    fig, ax1 = plt.subplots(figsize=(6.2, 4.0))
    ax1.plot(bfix, timeout, marker='o', color='tab:red', label='timeout ratio')
    ax1.set_xlabel('B_fix slots')
    ax1.set_ylabel('SLAC timeout ratio', color='tab:red')
    ax1.tick_params(axis='y', labelcolor='tab:red')
    ax1.grid(True, alpha=0.3)
    ax2 = ax1.twinx()
    ax2.plot(bfix, idle, marker='s', color='tab:blue', label='idle waste')
    ax2.set_ylabel('Mean idle reserved slots per period', color='tab:blue')
    ax2.tick_params(axis='y', labelcolor='tab:blue')
    fig.suptitle('B_fix sweep: timeout versus idle waste')
    save_both(out, 'fixed_by_bfix_timeout_and_waste')

    labels = ['admitted', 'rejected', 'completed', 'timed_out']
    values = [f(adaptive, 'admitted_slac_sessions'), f(adaptive, 'rejected_slac_sessions'), f(adaptive, 'completed_slac_sessions'), f(adaptive, 'timed_out_slac_sessions')]
    plt.figure(figsize=(5.4, 3.8))
    bars = plt.bar(labels, values, color=['tab:green', 'tab:gray', 'tab:blue', 'tab:red'])
    plt.ylabel('SLAC sessions')
    plt.title('Adaptive admission outcomes')
    plt.grid(axis='y', alpha=0.25)
    for bar, value in zip(bars, values):
        plt.text(bar.get_x() + bar.get_width() / 2, bar.get_height(), f'{int(value)}', ha='center', va='bottom', fontsize=9)
    save_both(out, 'adaptive_admission_outcomes')

    # Keep earlier exploratory figure names for compatibility with existing notes.
    plt.figure(figsize=(6.2, 4.0))
    plt.scatter(idle, timeout, label='fixed reservation')
    plt.scatter([adaptive_idle], [adaptive_timeout], marker='*', s=160, label='adaptive')
    plt.xlabel('idle waste')
    plt.ylabel('SLAC timeout ratio')
    plt.legend(frameon=False)
    plt.grid(True, alpha=0.3)
    save_both(out, 'exp2_tradeoff_curve')

if __name__ == '__main__':
    main()
