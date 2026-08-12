#!/usr/bin/env python3
import csv
import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


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
    result_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('results/exp3_condA_vs_condAB')
    data = read_rows(result_dir)
    out = result_dir / 'figures'
    table_path = result_dir / 'exp3_condA_vs_condAB_table.csv'
    with table_path.open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['scenario', 'N', 'K', 'cond_a_only_admit', 'cond_ab_admit', 'over_admission_count', 'regime', 'slack_slots'])
        writer.writeheader()
        for r in data:
            writer.writerow({k: r[k] for k in writer.fieldnames})

    labels = [r['scenario'] for r in data]
    over = [int(r['over_admission_count']) for r in data]
    cond_a = [int(r['cond_a_only_admit']) for r in data]
    cond_ab = [int(r['cond_ab_admit']) for r in data]
    x = range(len(data))
    plt.figure(figsize=(6.8, 3.8))
    plt.plot(x, cond_a, marker='o', label='Cond-A-only admit')
    plt.plot(x, cond_ab, marker='s', label='Cond-A+B admit')
    plt.bar(x, over, alpha=0.25, label='unsafe active-only admission')
    plt.xticks(list(x), labels, rotation=20, ha='right')
    plt.ylabel('indicator')
    plt.title('Exp3 Cond-A-only versus Cond-A+B')
    plt.legend(frameon=False)
    plt.grid(True, axis='y', alpha=0.3)
    save(out, 'exp3_condA_vs_condAB')

    plt.figure(figsize=(5.5, 3.6))
    y = [float(r.get('dc_response_time_slots') or 0) for r in data]
    plt.plot(list(range(len(y))), y, marker='o')
    plt.title('Exp3 response slots')
    plt.xlabel('sample')
    plt.ylabel('response time slots')
    plt.grid(True, alpha=0.3)
    save(out, 'exp3_plot')

if __name__ == '__main__':
    main()
