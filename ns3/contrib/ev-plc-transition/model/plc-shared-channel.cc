#include "plc-shared-channel.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

bool
PlcSharedChannel::IsBusyAt(uint64_t slot) const
{
    return slot < m_busyUntilSlot;
}

uint64_t
PlcSharedChannel::GetBusyUntilSlot() const
{
    return m_busyUntilSlot;
}

void
PlcSharedChannel::Occupy(uint64_t startSlot, uint32_t durationSlots)
{
    m_busyUntilSlot = std::max(m_busyUntilSlot, startSlot + durationSlots);
}

void
PlcSharedChannel::SetPer(double per)
{
    m_per = per;
}

void
PlcSharedChannel::SetRngSeed(uint32_t seed)
{
    m_rng.seed(seed);
}

void
PlcSharedChannel::SetReferenceSnrDb(double snrDb)
{
    m_referenceSnrDb = snrDb;
}

void
PlcSharedChannel::SetLinkAttenuationDb(uint32_t evId, double db)
{
    m_attenDb[evId] = db;
    m_matrixEnabled = true;
    if (m_snrPerTable.empty())
    {
        SetDefaultSnrTable();
    }
}

void
PlcSharedChannel::SetSnrPerTable(const std::vector<std::pair<double, double>>& table)
{
    m_snrPerTable = table;
    std::sort(m_snrPerTable.begin(), m_snrPerTable.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
}

void
PlcSharedChannel::SetDefaultSnrTable()
{
    // Sweep-placeholder defaults (no point-estimate claims): step table
    // (snr >= threshold -> slot PER). Calibration target: Layer 3 HIL.
    SetSnrPerTable({{30.0, 1e-6}, {20.0, 1e-5}, {15.0, 1e-4}, {10.0, 1e-3}, {5.0, 1e-2},
                    {-1e9, 1e-1}});
}

void
PlcSharedChannel::SetProfileAttenuationDb(PlcProfileClass profile, double db)
{
    m_profileAttenDb[profile] = db;
}

void
PlcSharedChannel::InitAttenuationFromProfiles(const std::vector<PlcProfileClass>& perEv)
{
    for (uint32_t ev = 0; ev < perEv.size(); ++ev)
    {
        const auto it = m_profileAttenDb.find(perEv[ev]);
        SetLinkAttenuationDb(ev, it != m_profileAttenDb.end() ? it->second : 20.0);
    }
}

void
PlcSharedChannel::EnableGilbertElliott(double pGoodToBad, double pBadToGood, double perGood,
                                       double perBad)
{
    m_geEnabled = true;
    m_pGoodToBad = pGoodToBad;
    m_pBadToGood = pBadToGood;
    m_perGood = perGood;
    m_perBad = perBad;
    m_geSegments.clear();
    m_geScanIdx = 0;
}

void
PlcSharedChannel::SetGeRngSeed(uint32_t seed)
{
    m_geRng.seed(seed);
}

double
PlcSharedChannel::LinkSlotPer(uint32_t evId) const
{
    if (!m_matrixEnabled)
    {
        return 0.0;
    }
    const auto it = m_attenDb.find(evId);
    const double snr = m_referenceSnrDb - (it != m_attenDb.end() ? it->second : 0.0);
    for (const auto& [threshold, pSlot] : m_snrPerTable)
    {
        if (snr >= threshold)
        {
            return pSlot;
        }
    }
    return m_snrPerTable.empty() ? 0.0 : m_snrPerTable.back().second;
}

double
PlcSharedChannel::LinkFramePer(uint32_t evId, uint32_t durationSlots) const
{
    return FrameFailureProbability(durationSlots, 0, LinkSlotPer(evId), 0.0);
}

double
PlcSharedChannel::ClassFramePer(PlcProfileClass profile, uint32_t durationSlots) const
{
    const auto it = m_profileAttenDb.find(profile);
    const double snr = m_referenceSnrDb - (it != m_profileAttenDb.end() ? it->second : 20.0);
    double pSlot = m_snrPerTable.empty() ? 0.0 : m_snrPerTable.back().second;
    for (const auto& [threshold, p] : m_snrPerTable)
    {
        if (snr >= threshold)
        {
            pSlot = p;
            break;
        }
    }
    return FrameFailureProbability(durationSlots, 0, pSlot, 0.0);
}

double
PlcSharedChannel::FrameFailureProbability(uint32_t goodSlots, uint32_t badSlots, double pGood,
                                          double pBad)
{
    return 1.0 - std::pow(1.0 - pGood, goodSlots) * std::pow(1.0 - pBad, badSlots);
}

void
PlcSharedChannel::AdvanceGeTimeline(uint64_t untilSlot)
{
    // Lazy sojourn sampling on the dedicated stream — no per-slot ticking.
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    while (m_geSegments.empty() || m_geSegments.back().endSlot < untilSlot)
    {
        const bool bad = m_geSegments.empty() ? false : !m_geSegments.back().bad;
        const uint64_t start = m_geSegments.empty() ? 0 : m_geSegments.back().endSlot;
        const double pLeave = bad ? m_pBadToGood : m_pGoodToBad;
        uint64_t sojourn = 1;
        if (pLeave > 0.0 && pLeave < 1.0)
        {
            const double u = uni(m_geRng);
            sojourn = 1 + static_cast<uint64_t>(std::floor(std::log(u) / std::log(1.0 - pLeave)));
        }
        else if (pLeave <= 0.0)
        {
            sojourn = untilSlot - start + 1; // absorbing for this query horizon
        }
        m_geSegments.push_back({bad, start, start + sojourn});
    }
}

std::pair<uint32_t, uint32_t>
PlcSharedChannel::GoodBadOverlap(uint64_t startSlot, uint32_t durationSlots)
{
    const uint64_t endSlot = startSlot + durationSlots;
    AdvanceGeTimeline(endSlot);
    uint32_t good = 0;
    uint32_t bad = 0;
    if (m_geScanIdx > 0 &&
        (m_geScanIdx > m_geSegments.size() || m_geSegments[m_geScanIdx - 1].endSlot > startSlot))
    {
        m_geScanIdx = 0;
    }
    while (m_geScanIdx < m_geSegments.size() && m_geSegments[m_geScanIdx].endSlot <= startSlot)
    {
        ++m_geScanIdx;
    }
    for (size_t idx = m_geScanIdx; idx < m_geSegments.size(); ++idx)
    {
        const auto& segment = m_geSegments[idx];
        const uint64_t lo = std::max(segment.startSlot, startSlot);
        const uint64_t hi = std::min(segment.endSlot, endSlot);
        if (hi > lo)
        {
            (segment.bad ? bad : good) += static_cast<uint32_t>(hi - lo);
        }
        if (segment.startSlot >= endSlot)
        {
            break;
        }
    }
    return {good, bad};
}

void
PlcSharedChannel::ForceGeSegmentsForTest(const std::vector<std::pair<bool, uint64_t>>& segments)
{
    m_geEnabled = true;
    m_geSegments.clear();
    m_geScanIdx = 0;
    uint64_t start = 0;
    for (const auto& [bad, endSlot] : segments)
    {
        m_geSegments.push_back({bad, start, endSlot});
        start = endSlot;
    }
}

bool
PlcSharedChannel::FrameFailsAt(uint32_t evId, uint64_t startSlot, uint32_t durationSlots)
{
    if (!m_geEnabled && !m_matrixEnabled)
    {
        // C1 legacy path, verbatim: single frame-level roll against m_per.
        return m_per > 0.0 && m_uni(m_rng) < m_per;
    }
    const double pLink = LinkSlotPer(evId);
    double pFail = 0.0;
    if (m_geEnabled)
    {
        const auto [goodSlots, badSlots] = GoodBadOverlap(startSlot, durationSlots);
        const double pGood = 1.0 - (1.0 - pLink) * (1.0 - m_perGood);
        const double pBad = 1.0 - (1.0 - pLink) * (1.0 - m_perBad);
        pFail = FrameFailureProbability(goodSlots, badSlots, pGood, pBad);
    }
    else
    {
        pFail = FrameFailureProbability(durationSlots, 0, pLink, 0.0);
    }
    return m_uni(m_rng) < pFail; // still exactly one roll on the frame stream
}

} // namespace ns3
