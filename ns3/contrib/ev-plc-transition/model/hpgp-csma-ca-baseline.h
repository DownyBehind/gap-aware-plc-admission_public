#ifndef HPGP_CSMA_CA_BASELINE_H
#define HPGP_CSMA_CA_BASELINE_H

#include "hpgp-contention-node.h"
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace ns3
{

struct HpgpTraceEvent
{
    uint64_t timeSlot{0};
    std::string eventType;
    uint32_t nodeId{0};
    HpgpTrafficType trafficType{HpgpTrafficType::OTHER};
    uint32_t attempt{0};
    uint32_t backoff{0};
    uint32_t cw{0};
    bool collision{false};
    bool retry{false};
    uint64_t frameStart{0};
    uint64_t frameEnd{0};
    bool success{false};
    std::string mediumState;
};

struct HpgpBaselineMetrics
{
    uint32_t successfulFrames{0};
    uint32_t collisions{0};
    uint32_t retries{0};
    uint32_t drops{0};
    double averageLatencySlots{0.0};
    uint64_t maxLatencySlots{0};
    uint32_t dcDeadlineMissCount{0};
};

class HpgpCsmaCaBaseline
{
  public:
    explicit HpgpCsmaCaBaseline(const HpgpCsmaCaParams& params = HpgpCsmaCaParams());

    void SetSeed(uint32_t seed);
    void SetTraceEnabled(bool enabled);
    void AddNode(uint32_t nodeId);
    void EnqueueFrame(uint32_t nodeId, HpgpFrame frame);
    void SetBackoffForTest(uint32_t nodeId, uint32_t backoff, uint32_t cw = 7, uint32_t dc = 0);
    void Step();
    void RunUntil(uint64_t endSlot);
    void RunUntilIdle(uint64_t maxSlots = 100000);
    void ForceMediumBusy(uint64_t untilSlot);

    const std::vector<HpgpTraceEvent>& GetTrace() const;
    HpgpBaselineMetrics GetMetrics() const;
    const HpgpContentionNode& GetNode(uint32_t nodeId) const;
    uint64_t GetCurrentSlot() const;

    void ExportTraceCsv(const std::string& path) const;

  private:
    bool IsMediumBusy() const;
    void Log(const HpgpTraceEvent& event);
    void ResolveContention();
    HpgpContentionNode& EnsureNode(uint32_t nodeId);

    HpgpCsmaCaParams m_params;
    std::map<uint32_t, HpgpContentionNode> m_nodes;
    std::mt19937 m_rng;
    uint64_t m_currentSlot{0};
    std::optional<uint32_t> m_transmittingNode;
    uint64_t m_mediumBusyUntil{0};
    std::vector<HpgpTraceEvent> m_trace;
    bool m_traceEnabled{true};
    uint32_t m_collisions{0};
};

} // namespace ns3

#endif // HPGP_CSMA_CA_BASELINE_H
