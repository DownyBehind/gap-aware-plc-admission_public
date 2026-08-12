#!/usr/bin/env python3
import csv, json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'results' / 'paper_figures'
TABLES = ROOT / 'results' / 'paper_tables'
OUT.mkdir(parents=True, exist_ok=True)
TABLES.mkdir(parents=True, exist_ok=True)

def rows(path):
    with Path(path).open(newline='') as f:
        return list(csv.DictReader(f))

def save(stem):
    plt.tight_layout(); plt.savefig(OUT / f'{stem}.png', dpi=300); plt.savefig(OUT / f'{stem}.pdf'); plt.close()

def exp1():
    sched = rows(ROOT/'results/exp1_csma_vs_scheduled/metrics.csv')
    hpgp = rows(ROOT/'results/exp1_csma_vs_scheduled/hpgp_csma_ca_like/metrics.csv')[0]
    scheduled = sorted(float(r['dc_response_time_slots']) for r in sched)
    # Slot-level HPGP baseline exposes aggregate latency; use avg/max as a compact empirical tail proxy.
    hpgp_lat = sorted([float(hpgp['average_latency_slots']), float(hpgp['max_latency_slots'])])
    plt.figure(figsize=(6.2,4.0))
    for vals, label in [(scheduled, 'scheduled-after-admission'), (hpgp_lat, 'HPGP CSMA/CA-like latency proxy')]:
        y=[(i+1)/len(vals) for i in range(len(vals))]
        plt.step(vals, y, where='post', label=label)
    plt.axvline(1381, color='black', linestyle='--', linewidth=1, label='T_sched deadline')
    plt.xlabel('Response / latency slots')
    plt.ylabel('CDF')
    plt.title('Exp1 response-tail comparison')
    plt.grid(True, alpha=0.3); plt.legend(frameon=False)
    save('exp1_response_tail_comparison')

    tr = rows(ROOT/'results/exp1_csma_vs_scheduled/hpgp_csma_ca_like/hpgp_csma_ca_trace.csv')
    colors={'DC_REQ':'tab:blue','DC_RES':'tab:green','SLAC':'tab:orange'}
    ymap={'DC_REQ':2,'DC_RES':1,'SLAC':0}
    plt.figure(figsize=(7.0,3.8))
    for r in tr:
        event=r['event_type']; typ=r['traffic_type']
        if event in {'tx_start','collision_retry'}:
            start=float(r['frame_start']); end=float(r['frame_end']) if float(r['frame_end'])>start else start+1
            y=ymap.get(typ,0)
            if event=='collision_retry':
                plt.plot([start,end],[y,y], color='tab:red', linewidth=4, alpha=0.7)
                plt.text(start, y+0.12, 'collision/retry', fontsize=7)
            else:
                plt.plot([start,end],[y,y], color=colors.get(typ,'gray'), linewidth=4)
    plt.yticks([0,1,2], ['SLAC','DC_RES','DC_REQ'])
    plt.xlabel('slot')
    plt.title('Exp1 HPGP-like contention trace')
    plt.grid(True, axis='x', alpha=0.3)
    save('exp1_contention_trace')

def exp2():
    fixed=rows(ROOT/'results/exp2_fixed_vs_adaptive/fixed_by_bfix_summary.csv')
    adaptive=rows(ROOT/'results/exp2_fixed_vs_adaptive/adaptive_summary.csv')[0]
    b=[int(float(r['B_fix'])) for r in fixed]
    idle=[float(r['mean_idle_waste_per_period']) for r in fixed]
    timeout=[float(r['slac_timeout_ratio']) for r in fixed]
    plt.figure(figsize=(6.3,4.0))
    plt.scatter(idle, timeout, s=70, label='fixed reservation family')
    for x,y,bb in zip(idle,timeout,b): plt.annotate(f'B={bb}', (x,y), xytext=(5,5), textcoords='offset points', fontsize=8)
    plt.scatter([float(adaptive['total_idle_waste'])], [0], marker='*', s=160, label='adaptive')
    plt.annotate('small B: high timeout', (idle[0], timeout[0]), xytext=(30,-20), textcoords='offset points', arrowprops={'arrowstyle':'->','lw':0.8}, fontsize=8)
    plt.annotate('very large B: DC pressure', (idle[-1], timeout[-1]), xytext=(-120,30), textcoords='offset points', arrowprops={'arrowstyle':'->','lw':0.8}, fontsize=8)
    plt.xlabel('Mean idle reserved slots per period'); plt.ylabel('SLAC timeout ratio')
    plt.title('Exp2 fixed reservation trade-off'); plt.grid(True, alpha=0.3); plt.legend(frameon=False)
    save('exp2_fixed_tradeoff_idle_vs_timeout')
    fig, ax1=plt.subplots(figsize=(6.3,4.0)); ax1.plot(b,timeout,marker='o',color='tab:red'); ax1.set_xlabel('B_fix slots'); ax1.set_ylabel('Timeout ratio',color='tab:red'); ax1.tick_params(axis='y',labelcolor='tab:red'); ax1.grid(True,alpha=0.3)
    ax2=ax1.twinx(); ax2.plot(b,idle,marker='s',color='tab:blue'); ax2.set_ylabel('Mean idle waste slots',color='tab:blue'); ax2.tick_params(axis='y',labelcolor='tab:blue')
    miss=[int(float(r['dc_deadline_miss_count'])) for r in fixed]
    for bb,m in zip(b,miss):
        if m>0: ax1.annotate(f'DC miss={m}', (bb,0), xytext=(-35,20), textcoords='offset points', fontsize=8)
    fig.suptitle('Exp2 B_fix sweep'); save('exp2_bfix_sweep')
    labels=['admitted','rejected','completed','timed out']; vals=[float(adaptive['admitted_slac_sessions']),float(adaptive['rejected_slac_sessions']),float(adaptive['completed_slac_sessions']),float(adaptive['timed_out_slac_sessions'])]
    plt.figure(figsize=(5.3,3.8)); bars=plt.bar(labels, vals, color=['tab:green','tab:gray','tab:blue','tab:red']); plt.title('Exp2 adaptive admission outcomes'); plt.ylabel('sessions'); plt.grid(axis='y',alpha=0.25)
    for bar,v in zip(bars,vals): plt.text(bar.get_x()+bar.get_width()/2, bar.get_height(), str(int(v)), ha='center', va='bottom', fontsize=8)
    plt.figtext(0.5,0.01,'Timeout=0 applies to admitted sessions; rejected sessions are counted separately.',ha='center',fontsize=8)
    save('exp2_adaptive_outcomes')

def exp3():
    out=TABLES/'exp3_condA_condB_counterexample.csv'
    with out.open('w', newline='') as f:
        w=csv.DictWriter(f, fieldnames=['N','K','CondA_LHS','CondA_Result','CondB_LHS','CondB_Result','CondAOnly_Action','CondAB_Action','Reason'])
        w.writeheader(); w.writerow({'N':36,'K':1,'CondA_LHS':'1354 <= 1381','CondA_Result':'PASS','CondB_LHS':'1389 > 1381','CondB_Result':'FAIL','CondAOnly_Action':'Admit','CondAB_Action':'Reject','Reason':'CondB_fail'})
    plt.figure(figsize=(7.2,1.8)); plt.axis('off')
    data=[['N','K','Cond A','Cond B','Cond-A-only','Cond A+B','Reason'],['36','1','1354 <= 1381 PASS','1389 > 1381 FAIL','Admit','Reject','CondB_fail']]
    tbl=plt.table(cellText=data, loc='center', cellLoc='center'); tbl.auto_set_font_size(False); tbl.set_fontsize(8); tbl.scale(1,1.5)
    plt.title('Exp3 active-safe does not imply post-transition-safe')
    save('exp3_condA_condB_counterexample')

def exp4():
    data=rows(ROOT/'results/exp4_three_regime/metrics.csv')
    ns=sorted({int(r['N']) for r in data}); ks=sorted({int(r['K']) for r in data})
    val={'hidden':0,'paid':1,'rejected':2}; grid=[[0 for _ in ns] for _ in ks]
    ni={n:i for i,n in enumerate(ns)}; ki={k:i for i,k in enumerate(ks)}
    paid_slack=[]
    for r in data:
        regime=r['regime'].lower(); grid[ki[int(r['K'])]][ni[int(r['N'])]]=val[regime]
        if regime=='paid': paid_slack.append(float(r['slack_slots']))
    plt.figure(figsize=(7.0,4.8)); im=plt.imshow(grid,origin='lower',aspect='auto',extent=[min(ns),max(ns),min(ks),max(ks)],vmin=0,vmax=2); cb=plt.colorbar(im,ticks=[0,1,2]); cb.ax.set_yticklabels(['hidden','paid','rejected']); plt.xlabel('N, DC-active EV count'); plt.ylabel('K, active SLAC count'); plt.title('Exp4 three-regime N-K heatmap'); save('exp4_three_regime_heatmap')
    deltas=[paid_slack[i]-paid_slack[i+1] for i in range(len(paid_slack)-1) if abs(paid_slack[i]-paid_slack[i+1])<80]
    plt.figure(figsize=(6.2,3.8)); plt.hist(deltas, bins=20, alpha=0.75); plt.axvline(28,color='tab:red',linestyle='--',label='theory DeltaF=28'); plt.xlabel('Observed slack degradation slots'); plt.ylabel('count'); plt.title('Exp4 slack degradation distribution'); plt.legend(frameon=False); plt.grid(True,axis='y',alpha=0.3); save('exp4_slack_degradation_distribution')

def main():
    exp1(); exp2(); exp3(); exp4()
if __name__=='__main__': main()
