#!/usr/bin/env python3
import csv, json, sys
from pathlib import Path

def read_metrics(result_dir):
    path = Path(result_dir) / 'metrics.csv'
    if not path.exists():
        return []
    with path.open(newline='') as f:
        return list(csv.DictReader(f))

def read_summary(result_dir):
    path = Path(result_dir) / 'summary.json'
    if not path.exists():
        return {}
    return json.loads(path.read_text())

def main():
    result_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('results/exp1_csma_vs_scheduled')
    rows = read_metrics(result_dir)
    summary = read_summary(result_dir)
    print(json.dumps({'rows': len(rows), 'summary': summary}, indent=2))

if __name__ == '__main__':
    main()
