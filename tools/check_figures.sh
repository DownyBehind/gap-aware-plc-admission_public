#!/usr/bin/env bash
# Figure reproduction gate.
#
# Default mode (environment-robust): regenerates the five committed figures
# (the paper's four figures plus the supplementary knee figure) and
# verifies content — make_fig3_region.py asserts its regenerated CSV is
# byte-identical to the committed copy, make_fig4_knee.py asserts the four
# knee sets read from committed CSVs, make_fig2_cycle.py asserts the builder
# values it draws — then checks all five PDFs exist and are non-empty.
#
# --strict additionally compares each bounding box against the shipped value
# (±0.05 pt). This is an internal typesetting gate: it only holds with
# matplotlib built against the system FreeType (tools/setup_python_env.sh)
# and is NOT required to validate the scientific content.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/figures/out"
STRICT=0
[ "${1:-}" = "--strict" ] && STRICT=1

declare -A SCRIPT=(
  [fig2_cycle_structure]=make_fig2_cycle.py
  [fig3_admission_region]=make_fig3_region.py
  [fig4_knee_verification]=make_fig4_knee.py
  [fig5_policy_comparison]=make_fig5_policy.py
)
declare -A EXPECT=(
  [fig2_cycle_structure]="514.395 115.0"
  [fig3_admission_region]="259.995 162.694"
  [fig4_knee_verification]="259.5 165.574"
  [fig5_policy_comparison]="367.79 129.185"
)

fail=0
for name in fig2_cycle_structure fig3_admission_region \
            fig4_knee_verification fig5_policy_comparison; do
  echo "[check_figures] ${SCRIPT[$name]}"
  python3 "$ROOT/figures/${SCRIPT[$name]}" >/dev/null
  pdf="$OUT/$name.pdf"
  if [ ! -s "$pdf" ]; then
    echo "  FAIL $name: output missing or empty" >&2
    fail=1
    continue
  fi
  if [ "$STRICT" -eq 1 ]; then
    read -r w h < <(pdfinfo "$pdf" | awk '/Page size/ {print $3, $5}')
    read -r ew eh <<< "${EXPECT[$name]}"
    ok=$(awk -v w="$w" -v h="$h" -v ew="$ew" -v eh="$eh" \
         'BEGIN { d1=w-ew; d2=h-eh; if (d1<0) d1=-d1; if (d2<0) d2=-d2;
                  print (d1<=0.05 && d2<=0.05) ? 1 : 0 }')
    if [ "$ok" = "1" ]; then
      echo "  PASS $name: ${w} x ${h} pt (expected ${ew} x ${eh})"
    else
      echo "  FAIL $name: ${w} x ${h} pt != expected ${ew} x ${eh}" >&2
      fail=1
    fi
  else
    echo "  PASS $name: generated ($(stat -c%s "$pdf") bytes)"
  fi
done

if [ "$fail" -ne 0 ]; then
  echo "[check_figures] FAILED" >&2
  exit 1
fi
if [ "$STRICT" -eq 1 ]; then
  echo "[check_figures] all 4 figures reproduce their shipped bounding boxes"
else
  echo "[check_figures] all 4 figures regenerate from committed data (content asserts passed)"
fi
