#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, enforce_response_dominant


def requests_first_finish(n: int, p: TransitionParams) -> int:
    return max(n * p.C_req_slots, p.C_req_slots + p.C_proc_slots) + n * p.C_res_slots + p.B_blk_slots


def delayed_request_finish(n: int, p: TransitionParams) -> int:
    # Conservative delayed-request schedule: each EV waits for processing before next request.
    return n * (p.C_req_slots + p.C_proc_slots + p.C_res_slots) + p.B_blk_slots


def interleaved_finish(n: int, p: TransitionParams) -> int:
    # Simple REQ/RES chain for the first EV, then remaining requests/responses.
    first_response_end = p.C_req_slots + p.C_proc_slots + p.C_res_slots
    remaining = max(0, n - 1) * (p.C_req_slots + p.C_res_slots)
    return first_response_end + remaining + p.B_blk_slots


def main() -> None:
    p = TransitionParams()
    enforce_response_dominant(p)
    for n in [1, 3, 10, 20]:
        assert p.C_res_slots >= p.C_req_slots
        assert requests_first_finish(n, p) <= delayed_request_finish(n, p)
        assert requests_first_finish(n, p) <= interleaved_finish(n, p) or n == 1
    try:
        enforce_response_dominant(TransitionParams(C_req_slots=30, C_res_slots=10))
    except ValueError:
        pass
    else:
        raise AssertionError("C_res < C_req must invalidate closed-form bound")


if __name__ == "__main__":
    main()
