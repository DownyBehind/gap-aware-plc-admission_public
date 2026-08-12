// Track C C4: G3 experiments.
//   --mode=b : link-aware vs count-based admission under a heterogeneous
//              attenuation matrix (half GOOD, half SEVERE; G-E OFF).
//   --mode=c : DC-miss burst sensitivity — EvPlcMac fixed (N,K) cells under
//              G-E with the marginal slot-PER held constant across the
//              (bad-sojourn, per_bad) plane.
//   --mode=a : q_wc=25/cap=3 D_g violation rate on the same G-E plane
//              (discovery recording only; burst-aware q rederivation is
//              explicitly out of scope).

#include "ns3/core-module.h"
#include "ns3/ev-plc-mac.h"
#include "ns3/ev-plc-params.h"
#include "ns3/ev-plc-policy-mac.h"
#include "ns3/plc-shared-channel.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;

namespace
{

// Marginal slot-PER target shared by the G-E plane (matches the ii-b i.i.d.
// baseline: frame PER 1e-3 on 15-21 slot frames ~ slot PER 6.7e-5).
constexpr double kTargetMarginalSlotPer = 6.7e-5;

void
ConfigureGe(Ptr<PlcSharedChannel> channel, double badSojourn, double perBad, uint32_t seed)
{
    const double duty = kTargetMarginalSlotPer / perBad; // bad-state fraction
    const double pBadToGood = 1.0 / badSojourn;
    const double goodSojourn = badSojourn * (1.0 / duty - 1.0);
    const double pGoodToBad = 1.0 / goodSojourn;
    channel->EnableGilbertElliott(pGoodToBad, pBadToGood, 0.0, perBad);
    channel->SetGeRngSeed(seed * 6871 + static_cast<uint32_t>(badSojourn) * 13 +
                          static_cast<uint32_t>(perBad * 1000));
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string mode = "b";
    uint32_t seeds = 20;
    std::string cellsArg;
    bool aggCap = false;
    CommandLine cmd(__FILE__);
    cmd.AddValue("mode", "b, c, a, or i (i.i.d. baseline)", mode);
    cmd.AddValue("seeds", "seed count", seeds);
    cmd.AddValue("cells",
                 "comma list N:K overriding the default cell set (modes i and c); "
                 "empty keeps the committed default {10:4,25:4,37:1}",
                 cellsArg);
    cmd.AddValue("aggCap", "enable the aggregate SLAC window cap", aggCap);
    cmd.Parse(argc, argv);

    std::vector<std::array<uint32_t, 2>> cellList = {{10, 4}, {25, 4}, {37, 1}};
    if (!cellsArg.empty())
    {
        cellList.clear();
        size_t pos = 0;
        while (pos < cellsArg.size())
        {
            const size_t comma = cellsArg.find(',', pos);
            const std::string tok = cellsArg.substr(
                pos, comma == std::string::npos ? std::string::npos : comma - pos);
            const size_t colon = tok.find(':');
            NS_ABORT_MSG_IF(colon == std::string::npos, "cells token needs N:K — " << tok);
            cellList.push_back({static_cast<uint32_t>(std::stoul(tok.substr(0, colon))),
                                static_cast<uint32_t>(std::stoul(tok.substr(colon + 1)))});
            pos = comma == std::string::npos ? cellsArg.size() : comma + 1;
        }
    }

    EvPlcParams params;

    if (mode == "b")
    {
        // Heterogeneous links: N0=10 DC EVs and K=16 sessions, half GOOD /
        // half SEVERE (SEVERE mapped to 35 dB -> SNR 5 -> slot PER 1e-2, so
        // SLAC retransmissions actually bite; parameter, not point estimate).
        const uint32_t n0 = 10;
        const uint32_t k = 16;
        std::cout << "variant,seed,admitted,never_admitted,wait_sum,dg_violations,"
                     "dg_severe,dg_good,admitted_severe,admitted_good,completed,"
                     "dc_misses,dc_ev_cycles\n";
        for (const std::string variant : {"count", "link_aware"})
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                channel->SetRngSeed(seed * 3271 + 11);
                channel->SetProfileAttenuationDb(PlcProfileClass::SEVERE, 35.0);
                std::vector<PlcProfileClass> dcClasses;
                for (uint32_t ev = 0; ev < n0; ++ev)
                {
                    dcClasses.push_back(ev % 2 == 0 ? PlcProfileClass::GOOD
                                                    : PlcProfileClass::SEVERE);
                }
                std::vector<PlcProfileClass> sessionClasses;
                for (uint32_t j = 0; j < k; ++j)
                {
                    sessionClasses.push_back(j % 2 == 0 ? PlcProfileClass::GOOD
                                                        : PlcProfileClass::SEVERE);
                }
                // Attenuation entries: DC EVs 0..n0-1, session links 100000+j.
                channel->InitAttenuationFromProfiles(dcClasses);
                for (uint32_t j = 0; j < k; ++j)
                {
                    channel->SetLinkAttenuationDb(
                        100000 + j, sessionClasses[j] == PlcProfileClass::GOOD ? 10.0 : 35.0);
                }
                Ptr<EvPlcPolicyMac> mac = Create<EvPlcPolicyMac>(params, channel);
                mac->SetAggregateCap(aggCap), mac->ConfigureAcbs(7, 0);
                mac->SetAdmissionVariant(variant == "count" ? PolicyAdmission::COUNT
                                                            : PolicyAdmission::LINK_AWARE);
                mac->SetErrorSource(PolicyErrorSource::CHANNEL, 0.0, 1);
                mac->ConfigureScenario(n0, k, dcClasses, sessionClasses);
                mac->Start(120);
                Simulator::Run();
                const auto stats = mac->GetStats();
                Simulator::Destroy();
                std::cout << variant << ',' << seed << ',' << stats.admitted << ','
                          << stats.neverAdmitted << ',' << stats.waitSumCycles << ','
                          << stats.dgViolations << ',' << stats.dgViolationsSevere << ','
                          << stats.dgViolationsGood << ',' << stats.admittedSevere << ','
                          << stats.admittedGood << ',' << stats.completed << ','
                          << stats.dcMisses << ',' << stats.dcEvCycles << "\n";
            }
        }
        return 0;
    }

    const double sojourns[] = {3, 15, 60, 300};
    const double perBads[] = {0.05, 0.2, 0.5};

    if (mode == "i")
    {
        // i.i.d. baseline of the G-E plane: the duty=1 limit, i.e. a uniform
        // slot-PER equal to the plane's marginal target. Realized by setting
        // p_good = p_bad = kTargetMarginalSlotPer (transition rates are then
        // irrelevant), so the mode-c cell loop applies unchanged.
        std::cout << "N,K,per_slot,miss_cycles,total_cycles\n";
        for (const auto& cell : cellList)
        {
            uint32_t miss = 0;
            uint32_t total = 0;
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                channel->SetRngSeed(seed * 4409 + cell[0] * 173 + cell[1] * 19);
                channel->EnableGilbertElliott(0.5, 0.5, kTargetMarginalSlotPer,
                                              kTargetMarginalSlotPer);
                channel->SetGeRngSeed(seed * 6871 + 7);
                Ptr<EvPlcMac> mac = Create<EvPlcMac>(params, channel);
                for (uint32_t ev = 0; ev < cell[0]; ++ev)
                {
                    mac->AddDcEv();
                }
                for (uint32_t j = 0; j < cell[1]; ++j)
                {
                    mac->AddSlacSession();
                }
                mac->Start(500);
                Simulator::Run();
                for (const auto& record : mac->GetRecords())
                {
                    total += 1;
                    if (record.finishSlot > params.GetScheduledSlots())
                    {
                        miss += 1;
                    }
                }
                Simulator::Destroy();
            }
            std::cout << cell[0] << ',' << cell[1] << ',' << kTargetMarginalSlotPer << ','
                      << miss << ',' << total << "\n";
        }
        return 0;
    }

    if (mode == "c")
    {
        std::cout << "N,K,bad_sojourn,per_bad,miss_cycles,total_cycles\n";
        for (const auto& cell : cellList)
        {
            for (const auto sojourn : sojourns)
            {
                for (const auto perBad : perBads)
                {
                    uint32_t miss = 0;
                    uint32_t total = 0;
                    for (uint32_t seed = 1; seed <= seeds; ++seed)
                    {
                        Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                        channel->SetRngSeed(seed * 4409 + cell[0] * 173 + cell[1] * 19);
                        ConfigureGe(channel, sojourn, perBad, seed);
                        Ptr<EvPlcMac> mac = Create<EvPlcMac>(params, channel);
                        for (uint32_t ev = 0; ev < cell[0]; ++ev)
                        {
                            mac->AddDcEv();
                        }
                        for (uint32_t j = 0; j < cell[1]; ++j)
                        {
                            mac->AddSlacSession();
                        }
                        mac->Start(500);
                        Simulator::Run();
                        for (const auto& record : mac->GetRecords())
                        {
                            total += 1;
                            if (record.finishSlot > params.GetScheduledSlots())
                            {
                                miss += 1;
                            }
                        }
                        Simulator::Destroy();
                    }
                    std::cout << cell[0] << ',' << cell[1] << ',' << sojourn << ',' << perBad
                              << ',' << miss << ',' << total << "\n";
                }
            }
        }
        return 0;
    }

    // mode a: q_wc burst limit — D_g violation rate on the same plane.
    std::cout << "bad_sojourn,per_bad,seed,admitted,dg_violations,completed\n";
    for (const auto sojourn : sojourns)
    {
        for (const auto perBad : perBads)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                channel->SetRngSeed(seed * 5563 + 7);
                ConfigureGe(channel, sojourn, perBad, seed);
                Ptr<EvPlcPolicyMac> mac = Create<EvPlcPolicyMac>(params, channel);
                mac->SetAggregateCap(aggCap), mac->ConfigureAcbs(25, 3);
                mac->SetErrorSource(PolicyErrorSource::CHANNEL, 0.0, 1);
                mac->ConfigureScenario(15, 8);
                mac->Start(120);
                Simulator::Run();
                const auto stats = mac->GetStats();
                Simulator::Destroy();
                std::cout << sojourn << ',' << perBad << ',' << seed << ',' << stats.admitted
                          << ',' << stats.dgViolations << ',' << stats.completed << "\n";
            }
        }
    }
    return 0;
}
