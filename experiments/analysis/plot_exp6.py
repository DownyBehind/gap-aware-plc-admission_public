#!/usr/bin/env python3
import csv, sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
ROOT=Path(__file__).resolve().parents[2]
def rows(p):
    with Path(p).open(newline='') as f: return list(csv.DictReader(f))
def save(out,stem):
    out.mkdir(parents=True,exist_ok=True); plt.tight_layout(); plt.savefig(out/f'{stem}.png',dpi=300); plt.savefig(out/f'{stem}.pdf'); plt.close()
def agg(rows, key, mode=None):
    vals=[float(r[key]) for r in rows if mode is None or r['mode']==mode]; return sum(vals)
def main():
    result=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'results/exp6_dynamic_arrivals'; out=ROOT/'results/paper_figures'
    ts=rows(result/'time_series.csv'); ms=rows(result/'mode_summary.csv'); modes=['hpgp_csma_ca_like','fixed_reservation','adaptive_transition_aware']
    rep=[r for r in ts if r['mode']=='adaptive_transition_aware' and r['arrival_pattern']=='depot_burst' and r['seed']=='1'][:500]
    plt.figure(figsize=(7.0,4.0)); plt.plot([float(r['time_s']) for r in rep],[float(r['N']) for r in rep],label='N(t) DC-active'); plt.plot([float(r['time_s']) for r in rep],[float(r['K']) for r in rep],label='K(t) active SLAC'); plt.xlabel('time (s)'); plt.ylabel('count'); plt.title('Exp6 dynamic workload N(t), K(t)'); plt.legend(frameon=False); plt.grid(True,alpha=0.3); save(out,'exp6_time_series_NK')
    metrics=['dc_deadline_miss_count','timed_out_slac_count','rejected_count','completed_slac_count']; x=range(len(modes)); width=0.2; plt.figure(figsize=(7.2,4.2))
    for i,m in enumerate(metrics): plt.bar([v+(i-1.5)*width for v in x],[agg(ms,m,mode) for mode in modes],width,label=m)
    plt.xticks(list(x),modes,rotation=12,ha='right'); plt.ylabel('count'); plt.title('Exp6 mode comparison'); plt.legend(fontsize=8,frameon=False); plt.grid(axis='y',alpha=0.25); save(out,'exp6_mode_comparison')
    labels=['admitted','rejected','completed','timed out']; keys=['admitted_count','rejected_count','completed_slac_count','timed_out_slac_count']; plt.figure(figsize=(7.0,4.0)); bottom=[0]*len(modes)
    for key,label in zip(keys,labels):
        vals=[agg(ms,key,mode) for mode in modes]; plt.bar(modes,vals,bottom=bottom,label=label); bottom=[b+v for b,v in zip(bottom,vals)]
    plt.xticks(rotation=12,ha='right'); plt.ylabel('sessions'); plt.title('Exp6 admission outcomes'); plt.legend(frameon=False); plt.grid(axis='y',alpha=0.25); save(out,'exp6_admission_outcomes')
    offered=[max(1,agg(ms,'offered_arrivals',mode)) for mode in modes]; miss=[agg(ms,'dc_deadline_miss_count',mode)/len([r for r in ms if r['mode']==mode]) for mode in modes]; tout=[agg(ms,'timed_out_slac_count',mode)/off for mode,off in zip(modes,offered)]
    plt.figure(figsize=(6.5,4.0)); plt.plot(modes,miss,marker='o',label='deadline misses per run'); plt.plot(modes,tout,marker='s',label='SLAC timeout ratio'); plt.xticks(rotation=12,ha='right'); plt.ylabel('ratio / count'); plt.title('Exp6 deadline and timeout summary'); plt.legend(frameon=False); plt.grid(True,alpha=0.3); save(out,'exp6_deadline_timeout_summary')
if __name__=='__main__': main()
