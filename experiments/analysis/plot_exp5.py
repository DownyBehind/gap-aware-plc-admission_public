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
def main():
    result=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'results/exp5_plc_degradation_sensitivity'
    out=ROOT/'results/paper_figures'
    data=rows(result/'metrics.csv'); summary=rows(result/'profile_summary.csv')
    profiles=['good','nominal','degraded','severe']; val={'hidden':0,'paid':1,'rejected':2}
    fig,axes=plt.subplots(2,2,figsize=(8.0,6.0),sharex=True,sharey=True); axes=axes.ravel()
    for ax,prof in zip(axes,profiles):
        pr=[r for r in data if r['profile']==prof]; ns=sorted({int(r['N']) for r in pr}); ks=sorted({int(r['K']) for r in pr}); ni={n:i for i,n in enumerate(ns)}; ki={k:i for i,k in enumerate(ks)}; grid=[[0 for _ in ns] for _ in ks]
        for r in pr: grid[ki[int(r['K'])]][ni[int(r['N'])]]=val[r['regime'].lower()]
        im=ax.imshow(grid,origin='lower',aspect='auto',extent=[min(ns),max(ns),min(ks),max(ks)],vmin=0,vmax=2); ax.set_title(prof); ax.set_xlabel('N'); ax.set_ylabel('K')
    fig.colorbar(im, ax=axes.tolist(), ticks=[0,1,2], label='0 hidden / 1 paid / 2 rejected')
    fig.suptitle('Exp5 PLC profile admissible regions'); save(out,'exp5_plc_profile_admissible_region')
    hidden=[int(r['hidden_count']) for r in summary]; paid=[int(r['paid_count']) for r in summary]; rejected=[int(r['rejected_count']) for r in summary]; x=range(len(profiles))
    plt.figure(figsize=(6.2,4.0)); plt.bar(x,hidden,label='hidden'); plt.bar(x,paid,bottom=hidden,label='paid'); plt.bar(x,rejected,bottom=[h+p for h,p in zip(hidden,paid)],label='rejected'); plt.xticks(x,profiles); plt.ylabel('grid states'); plt.title('Exp5 regime counts by PLC profile'); plt.legend(frameon=False); plt.grid(axis='y',alpha=0.3); save(out,'exp5_plc_profile_counts')
    plt.figure(figsize=(5.8,3.8)); plt.plot(profiles,[int(r['max_admissible_N']) for r in summary],marker='o'); plt.xlabel('PLC profile'); plt.ylabel('max admissible N'); plt.title('Exp5 max admissible N by PLC profile'); plt.grid(True,alpha=0.3); save(out,'exp5_max_admissible_N')
if __name__=='__main__': main()
