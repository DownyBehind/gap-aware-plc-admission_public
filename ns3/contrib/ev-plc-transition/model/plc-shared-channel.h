#ifndef PLC_SHARED_CHANNEL_H
#define PLC_SHARED_CHANNEL_H

// Track C layer-2 channel object: half-duplex occupancy arbitration plus the
// frame error process. Error model composition (C2):
//
//   p_link[ev]        = SnrPerTable(referenceSnrDb - attenDb[ev])   (i.i.d. link floor)
//   p_state           = perGood / perBad (global Gilbert-Elliott state)
//   p_slot(ev, s)     = 1 - (1 - p_link[ev]) * (1 - p_state)        (independent sources)
//   P_fail(frame)     = 1 - (1-p_slot(ev,g))^Lg * (1-p_slot(ev,b))^Lb
//
// with (Lg, Lb) the closed-form overlap of the frame interval with the G-E
// sojourn segments (normative rule: docs/model/physics_rules.md (G-E rules)). The G-E
// timeline advances lazily by sojourn sampling — no per-slot ticking — from a
// DEDICATED RNG stream (m_geRng), so enabling/advancing G-E never perturbs
// the frame-roll stream (m_rng): G2 bit parity and G3 seed reproducibility.
// The C1 constant-PER mode (single frame-level roll on m_rng) is taken
// verbatim when neither the matrix/table nor G-E is enabled.
//
// All mapping values (SNR table, profile-to-SNR, G-E parameters) are settable
// parameter tables with sweep-placeholder defaults — no point estimates
// (TRACKC_PLAN §1.4).
//
// Event-only class: excluded from the standalone build (build-list
// separation, no #ifdef — TRACKC_PLAN §1.2).

#include "plc-link-profile.h"

#include "ns3/simple-ref-count.h"

#include <cstdint>
#include <map>
#include <random>
#include <utility>
#include <vector>

namespace ns3
{

class PlcSharedChannel : public SimpleRefCount<PlcSharedChannel>
{
  public:
    PlcSharedChannel() = default;

    // Half-duplex occupancy in integer slots (Time conversion happens only at
    // the MAC scheduling boundary).
    bool IsBusyAt(uint64_t slot) const;
    uint64_t GetBusyUntilSlot() const;
    void Occupy(uint64_t startSlot, uint32_t durationSlots);

    // --- constant-PER legacy mode (C1; bit-parity path) ---
    void SetPer(double per);
    void SetRngSeed(uint32_t seed);

    // --- attenuation matrix + SNR -> slot-PER table (C2) ---
    void SetReferenceSnrDb(double snrDb);
    void SetLinkAttenuationDb(uint32_t evId, double db);
    // Rows (snrDbAtLeast, pSlot), evaluated as a step function from the
    // highest threshold downwards; defaults installed by SetDefaultSnrTable.
    void SetSnrPerTable(const std::vector<std::pair<double, double>>& table);
    void SetDefaultSnrTable();
    // Profile-class -> attenuation dB parameter map (reuses PlcProfileClass).
    void InitAttenuationFromProfiles(const std::vector<PlcProfileClass>& perEv);
    void SetProfileAttenuationDb(PlcProfileClass profile, double db);

    // --- Gilbert-Elliott burst process (C2; default OFF) ---
    void EnableGilbertElliott(double pGoodToBad, double pBadToGood, double perGood, double perBad);
    void SetGeRngSeed(uint32_t seed);

    // Frame-level failure decision: exactly one roll on m_rng per attempt in
    // every mode (stream discipline).
    bool FrameFailsAt(uint32_t evId, uint64_t startSlot, uint32_t durationSlots);

    // Frame-level error probabilities for admission-side inflation (G3-(b)):
    // no RNG consumption, pure lookups through the SNR table.
    double LinkFramePer(uint32_t evId, uint32_t durationSlots) const;
    double ClassFramePer(PlcProfileClass profile, uint32_t durationSlots) const;

    // Closed form 1 - (1-pg)^Lg (1-pb)^Lb (unit-tested against hand values).
    static double FrameFailureProbability(uint32_t goodSlots, uint32_t badSlots, double pGood,
                                          double pBad);
    // (Lg, Lb) overlap of [startSlot, startSlot+duration) with the G-E
    // timeline; advances the lazy timeline as needed.
    std::pair<uint32_t, uint32_t> GoodBadOverlap(uint64_t startSlot, uint32_t durationSlots);
    // Test hook: install an explicit segment schedule (state, endSlot).
    void ForceGeSegmentsForTest(const std::vector<std::pair<bool, uint64_t>>& segments);

  private:
    struct GeSegment
    {
        bool bad{false};
        uint64_t startSlot{0};
        uint64_t endSlot{0};
    };

    void AdvanceGeTimeline(uint64_t untilSlot);
    double LinkSlotPer(uint32_t evId) const;

    uint64_t m_busyUntilSlot{0};
    double m_per{0.0};
    std::mt19937 m_rng{1};
    std::uniform_real_distribution<double> m_uni{0.0, 1.0};

    bool m_matrixEnabled{false};
    double m_referenceSnrDb{40.0};
    std::map<uint32_t, double> m_attenDb;
    std::vector<std::pair<double, double>> m_snrPerTable; // sorted desc by threshold
    std::map<PlcProfileClass, double> m_profileAttenDb{
        {PlcProfileClass::GOOD, 10.0}, {PlcProfileClass::NOMINAL, 20.0},
        {PlcProfileClass::SEVERE, 30.0}};

    bool m_geEnabled{false};
    double m_pGoodToBad{0.0};
    double m_pBadToGood{0.0};
    double m_perGood{0.0};
    double m_perBad{0.0};
    std::mt19937 m_geRng{1}; // dedicated stream — never mixed with m_rng
    std::vector<GeSegment> m_geSegments;
    // First segment that can still overlap a query. Queries arrive in
    // nondecreasing startSlot order on the single busy timeline, so the scan
    // start only moves forward; a backwards query resets it (correctness
    // never depends on monotonicity, only the speedup does).
    size_t m_geScanIdx = 0;
};

} // namespace ns3

#endif // PLC_SHARED_CHANNEL_H
