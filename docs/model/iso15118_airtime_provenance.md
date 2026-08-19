# ISO 15118-2 CurrentDemand Airtime Provenance

## 1. Purpose

This document records the validation that `C_req = 15` and `C_res = 21`
slots are **conservative analysis bounds**: encoded ISO 15118-2
CurrentDemandReq/Res reference frames, converted with the paper's
byte-to-airtime model, remain within them. It is **not** a claim that
every CurrentDemand message has a fixed size, nor that 15/21 are
protocol-derived airtimes.

## 2. Scope

Only the reference profiles enumerated below are covered. EXI encoding
is value- and option-dependent — enabling different optional elements or
using different field values changes the byte count — so the results
here are **not a universal protocol maximum** over all schema-valid
messages.

## 3. Reference profiles

Machine-readable form (all enabled fields and their values):
`experiments/provenance/iso15118_profiles.json`. Summary:

| message | profile | fields |
|---|---|---|
| CurrentDemandReq | core | DC_EVStatus{EVReady=true, EVErrorCode=NO_ERROR, EVRESSSOC=55}, EVTargetCurrent{0,A,60}, ChargingComplete=false, EVTargetVoltage{0,V,450} — exactly the ISO 15118-2:2014 Annex D.2.3 example |
| CurrentDemandReq | conservative | core + EVMaximumVoltageLimit{0,V,1000} + EVMaximumCurrentLimit{0,A,400} + EVMaximumPowerLimit{3,W,350} + BulkChargingComplete=false + RemainingTimeToFullSoC{0,s,28800} + RemainingTimeToBulkSoC{0,s,14400} (every schema-optional element of CurrentDemandReqType enabled) |
| CurrentDemandRes | core | ResponseCode=OK, DC_EVSEStatus{NotificationMaxDelay=0, EVSENotification=None, EVSEStatusCode=EVSE_Ready}, EVSEPresentVoltage{0,V,450}, EVSEPresentCurrent{0,A,60}, EVSECurrentLimitAchieved=false, EVSEVoltageLimitAchieved=false, EVSEPowerLimitAchieved=false, EVSEID="FRA23E45B78C", SAScheduleTupleID=1 |
| CurrentDemandRes | conservative | core + DC_EVSEStatus.EVSEIsolationStatus=Valid + EVSEMaximumVoltageLimit{0,V,1000} + EVSEMaximumCurrentLimit{0,A,400} + EVSEMaximumPowerLimit{3,W,350} + MeterInfo{MeterID="DE…MTR01" (32 chars), MeterReading=123456789012, SigMeterReading=64 B, MeterStatus=1, TMeter=1774000000} + ReceiptRequired=true (every schema-optional element of CurrentDemandResType and its members enabled) |

All messages carry the Annex D header SessionID `3031323334353637`.
PhysicalValueType triplets are written {Multiplier, Unit, Value}.

## 4. Encoder provenance

- Library: **EVerest libcbv2g**, commit
  `03350be048b35b179905129005a97144a4bdcf93`
  (cbexigen-generated EXI codec for the ISO 15118-2:2014 Ed.1 schema,
  namespace `urn:iso:15118:2:2013:MsgDef`).
- Harness: `experiments/provenance/encode_currentdemand.c`.
- Commands:
  `cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && cmake --build build`,
  then
  `gcc -O2 -I <libcbv2g>/include encode_currentdemand.c libcbv2g_iso2.a libcbv2g_exi_codec.a -o encode_currentdemand && ./encode_currentdemand`.
- Encoder validation: the CurrentDemandReq **core** profile reproduces
  the EXI stream of **ISO 15118-2:2014 Annex D.2.3 (V2G message
  example 15) byte-for-byte** — 25 bytes,
  `8098020C0C4C8CCD0D4D8DD0D1001B8186078410C40C203000`. Annex D
  contains no CurrentDemandRes EXI example; the Res encodings rest on
  the same validated codec.

## 5. Encoded sizes

`results/provenance/iso15118_currentdemand_sizes.csv`:

| message | profile | EXI bytes |
|---|---|---:|
| CurrentDemandReq | core | 25 |
| CurrentDemandReq | conservative | 48 |
| CurrentDemandRes | core | 42 |
| CurrentDemandRes | conservative | 169 |

## 6. Stack overhead

`L_stack = 82 B = Ethernet 14 + IPv6 40 + TCP 20 + V2GTP 8`
(base header sizes; V2GTP header per ISO 15118-2 clause 7.8.3).
**TCP options and VLAN tagging are not modeled**; the 82-B figure is a
construction assumption, not a measured trace.

## 7. Airtime conversion

```
slots = ceil( (t_fixed + 8 · (L_EXI + L_stack) / R) / Δ )
Δ = 35.84 µs, L_stack = 82 B, t_fixed = 350 µs
R = 10 Mbps (reference profile) and 9.8452 Mbps (HPGP HS-ROBO)
```

**`t_fixed = 350 µs` remains an assumption** — a fixed per-frame PHY
overhead of the construction model, not a standards-derived constant
(HomePlug 1.0/AV conventions give ≈369.4 µs, IEEE 1901 defaults
≈547.6 µs). Closing the EXI payload side does not turn the PHY overhead
into a standards constant.

## 8. Validation result

Check script: `experiments/analysis/iso15118_airtime_check.py`
(asserts slots ≤ bound at R = 10 Mbps, the paper's reference profile;
the HS-ROBO rate is reported as sensitivity).

Pass lines implied by the bounds: at 10 Mbps, C_req = 15 admits
L_EXI ≤ 152 B and C_res = 21 admits L_EXI ≤ 421 B; at 9.8452 Mbps,
148 B and 413 B.

| message | profile | EXI B | total B | slots @10 Mbps | slots @9.8452 Mbps | bound | verdict |
|---|---|---:|---:|---:|---:|---:|---|
| CurrentDemandReq | core | 25 | 107 | 13 | 13 | 15 | PASS / PASS |
| CurrentDemandReq | conservative | 48 | 130 | 13 | 13 | 15 | PASS / PASS |
| CurrentDemandRes | core | 42 | 124 | 13 | 13 | 21 | PASS / PASS |
| CurrentDemandRes | conservative | 169 | 251 | 16 | 16 | 21 | PASS / PASS |

All evaluated reference encodings remain within the published analysis
bounds at both rates. (Independently of the EXI payload, the 10-Mbps
rate rounding itself remains load-bearing for C_req: at exactly
9.8452 Mbps the historical 150-B construction frame crosses to 16
slots — see docs/PARAMETER_PROVENANCE.md, Calibration limits.)

## 9. Limitations

- Reference-profile validation only: not a claim about every
  schema-valid CurrentDemand message (arbitrary-length elements such as
  larger MeterID strings or other messages are out of scope).
- Not a claim about any particular HPGP implementation's framing,
  aggregation, or retransmission behaviour.
- `t_fixed` and `L_stack` remain construction assumptions (Secs. 6–7).
- The encoder is one validated implementation (libcbv2g); the Annex D
  byte-match validates it on the Req core profile only.

## 10. Cross-reference: three different constants

The DC frame bounds and the SLAC provisioning budget have different
meanings and must not be conflated:

- `C_req = 15` / `C_res = 21` — **conservative airtime analysis
  bounds** that contain the encoded ISO 15118-2 reference frames
  (this document).
- `C_slac = 247` — **architecture parameter** of the coordinated
  scheduler: the per-session provisioning budget the reserving lineage
  serves (docs/model/slac_sequence_model.md, docs/PARAMETER_PROVENANCE.md).
- `Σℓ_i = 230` — the slot demand of the corrected 19-message SLAC
  sequence itself, the quantity the replay lineage plays.
