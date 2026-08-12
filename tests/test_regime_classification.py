#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, classify_regime, cond_A, cond_B


def main() -> None:
    p = TransitionParams()
    assert classify_regime(1, 1, p) == "hidden"
    assert classify_regime(20, 1, p) in {"paid", "rejected"}
    assert classify_regime(36, 2, p) == "paid"
    assert classify_regime(37, 2, p) == "rejected"
    assert cond_A(37, 1, p) is True
    assert cond_B(37, 1, p) is False


if __name__ == "__main__":
    main()
