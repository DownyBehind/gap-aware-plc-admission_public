#!/usr/bin/env python3
import csv, sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def rows(path):
    with (Path(path) / 'metrics.csv').open(newline='') as f:
        return list(csv.DictReader(f))

def save_plot(result_dir, name, title, xvals, yvals, ylabel):
    out = Path(result_dir) / 'figures'
    out.mkdir(parents=True, exist_ok=True)
    plt.figure()
    plt.plot(xvals, yvals, marker='o')
    plt.title(title)
    plt.xlabel('sample')
    plt.ylabel(ylabel)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(out / name)
    plt.close()

def main():
    result_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('results/exp1')
    data = rows(result_dir)
    y = [float(r.get('dc_response_time_slots') or r.get('channel_occupancy') or r.get('slack_slots') or r.get('over_admission_count') or 0) for r in data]
    save_plot(result_dir, 'exp1_plot.png', 'Exp 1 Response Tail', list(range(len(y))), y, 'metric')

if __name__ == '__main__':
    main()
