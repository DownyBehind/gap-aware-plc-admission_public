#!/usr/bin/env python3

PROFILES = {
    "GOOD": (12, 18, 21),
    "NOMINAL": (15, 21, 21),
    "SEVERE": (30, 45, 60),
}


def total_demand(n: int, mix: str) -> tuple[int, int]:
    if mix == "mostly_good":
        profiles = ["GOOD"] * n
    elif mix == "severe_heavy":
        profiles = ["SEVERE"] * n
    else:
        profiles = ["GOOD", "NOMINAL", "SEVERE"] * ((n + 2) // 3)
    req = sum(PROFILES[p][0] for p in profiles[:n])
    res = sum(PROFILES[p][1] for p in profiles[:n])
    return req, res


def count_based_finish(n: int) -> int:
    return max(n * 15, 295) + n * 21 + 21


def link_aware_finish(n: int, mix: str) -> int:
    req, res = total_demand(n, mix)
    return max(req, 295) + res + 60


def main() -> None:
    n = 30
    assert total_demand(n, "mostly_good") != total_demand(n, "severe_heavy")
    assert count_based_finish(n) == count_based_finish(n)
    assert link_aware_finish(n, "mostly_good") != link_aware_finish(n, "severe_heavy")
    assert max(n * 30, 295) + n * 45 + 60 >= link_aware_finish(n, "mostly_good")


if __name__ == "__main__":
    main()
