#!/usr/bin/env python3


def fixed_metrics(b_fix: int, slac_work: int = 247, active_sessions: int = 20) -> dict[str, float]:
    periods_needed = (slac_work + b_fix - 1) // b_fix
    timeout = periods_needed * 50 > 2000
    idle_waste = max(0, b_fix - active_sessions * 7)
    dc_pressure = b_fix >= 512
    return {"timeout": float(timeout), "idle_waste": float(idle_waste), "dc_pressure": float(dc_pressure)}


def main() -> None:
    small = fixed_metrics(8)
    mid = fixed_metrics(64)
    large = fixed_metrics(512)
    assert small["timeout"] == 0.0
    assert mid["idle_waste"] >= small["idle_waste"]
    assert large["idle_waste"] > mid["idle_waste"]
    assert large["dc_pressure"] == 1.0
    stress = fixed_metrics(4)
    assert stress["timeout"] == 1.0


if __name__ == "__main__":
    main()
