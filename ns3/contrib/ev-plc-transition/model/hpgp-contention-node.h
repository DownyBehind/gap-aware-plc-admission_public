#ifndef HPGP_CONTENTION_NODE_H
#define HPGP_CONTENTION_NODE_H

#include "hpgp-csma-ca-params.h"
#include "hpgp-frame.h"
#include <deque>
#include <optional>
#include <random>
#include <vector>

namespace ns3
{

enum class HpgpNodeState
{
    IDLE,
    CONTENDING,
    TRANSMITTING,
    DROPPED
};

class HpgpContentionNode
{
  public:
    explicit HpgpContentionNode(uint32_t nodeId = 0);

    void EnqueueFrame(const HpgpFrame& frame);
    void StartBackoff(const HpgpCsmaCaParams& params, std::mt19937& rng);
    void SetBackoffForTest(uint32_t backoff, uint32_t cw = 7, uint32_t dc = 0);
    void TickIdle();
    void TickBusyFreeze();
    bool IsReadyToTransmit() const;
    void StartTransmission(uint64_t startSlot);
    bool HandleCollision(const HpgpCsmaCaParams& params, std::mt19937& rng, uint64_t nowSlot);
    HpgpFrame HandleSuccess(uint64_t endSlot);

    uint32_t GetNodeId() const;
    uint32_t GetBackoff() const;
    uint32_t GetCw() const;
    uint32_t GetAttempt() const;
    uint32_t GetRetryCount() const;
    uint32_t GetDropCount() const;
    uint32_t GetSuccessCount() const;
    HpgpNodeState GetState() const;
    const HpgpFrame* PeekFrame() const;
    bool HasFrame() const;

  private:
    uint32_t DrawBackoff(uint32_t cw, std::mt19937& rng) const;

    uint32_t m_nodeId{0};
    HpgpNodeState m_state{HpgpNodeState::IDLE};
    std::deque<HpgpFrame> m_queue;
    std::optional<HpgpFrame> m_current;
    uint32_t m_bpc{0};
    uint32_t m_dc{0};
    uint32_t m_backoff{0};
    uint32_t m_cw{0};
    uint32_t m_retryCount{0};
    uint32_t m_dropCount{0};
    uint32_t m_successCount{0};
};

} // namespace ns3

#endif // HPGP_CONTENTION_NODE_H
