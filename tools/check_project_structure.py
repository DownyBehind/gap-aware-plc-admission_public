#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
required = [
'README.md','DEVELOPMENT.md','LICENSE','requirements.txt',
'src/formulas/transition_formulas.py','src/sim/cycle_builder.py',
'docs/model/equations.md','docs/model/physics_rules.md',
'docs/model/slac_sequence_model.md','docs/model/plc_link_profiles.md',
'ns3/module_path.txt','ns3/standalone/build.sh',
'ns3/contrib/ev-plc-transition/CMakeLists.txt',
'experiments/README.md','experiments/configs/README.md',
'experiments/run/README.md','experiments/analysis/README.md',
'experiments/expected/README.md','experiments/expected/expected_metrics.md',
'experiments/layer1/verify_knee_e1.py','experiments/layer1/sweep_admission_region.py',
'experiments/ns3_e1/run_e2.py','experiments/ns3_e1/run_trackc_c4.py',
'tests/README.md','tests/run_cpp_tests.sh','tests/run_python_tests.sh','tests/run_all_tests.sh',
'results/RESULTS.md','results/layer1/knee_verification.csv',
'results/ns3_e1/e2_admission_variants.csv',
'figures/make_fig2_cycle.py',
'figures/make_fig3_region.py','figures/make_fig4_knee.py','figures/make_fig5_policy.py',
'tools/clean_results.sh','tools/sync_ns3_module.sh','tools/setup_ns3.sh',
'tools/check_figures.sh','tools/setup_python_env.sh']
missing = [p for p in required if not (ROOT / p).exists()]
if missing:
    print('Missing required files:')
    for p in missing: print(f'- {p}')
    sys.exit(1)
module_vars = dict(line.strip().split('=',1) for line in (ROOT/'ns3/module_path.txt').read_text().splitlines() if '=' in line)
for key in ['NS3_ROOT','NS3_MODULE','BUILD_SYSTEM','NS3_MODULE_SOURCE']:
    if key not in module_vars:
        print(f'Missing module path key: {key}')
        sys.exit(1)
print('Project structure OK')
