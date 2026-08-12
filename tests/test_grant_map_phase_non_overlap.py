#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams
from src.sim.cycle_builder import build_cycle


def main() -> None:
    source = (ROOT / "ns3" / "contrib" / "ev-plc-transition" / "model" / "grant-map-scheduler.cc").read_text()
    assert '{"O_map"' not in source
    assert '"DC_REQ", start' in source
    assert '"SLAC_SERVICE", start' in source
    assert '"DC_RES", responseStart' in source

    # Invariant (c): phases are time-ordered and non-overlapping, PKT_GUARD
    # included, across representative states (empty, setup-only, DC-only,
    # hidden, paid, counterexample, saturated).
    p = TransitionParams()
    for n, k in [(0, 0), (0, 3), (5, 0), (5, 2), (20, 1), (37, 1), (40, 20)]:
        cycle = build_cycle(n, k, p)
        for a, b in zip(cycle.phases, cycle.phases[1:]):
            assert a.end <= b.start, f"overlap at ({n},{k}): {a} vs {b}"
        names = [ph.name for ph in cycle.phases]
        if k > 0:
            assert "SLAC_SERVICE" in names and "PKT_GUARD" in names
            assert names.index("PKT_GUARD") == names.index("SLAC_SERVICE") + 1


if __name__ == "__main__":
    main()
