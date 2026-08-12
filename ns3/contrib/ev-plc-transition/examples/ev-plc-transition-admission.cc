#include "ns3/core-module.h"
#include "ns3/ev-plc-sim-helper.h"
#include <fstream>
#include <cmath>
#include <regex>
#include <sstream>

using namespace ns3;

static bool
ReadNumber(const std::string& text, const std::string& key, double& value)
{
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([-+0-9.eE]+)");
    std::smatch match;
    if (std::regex_search(text, match, pattern))
    {
        value = std::stod(match[1]);
        return true;
    }
    return false;
}

static void
ApplyConfig(const std::string& configPath, EvPlcParams& params, uint32_t& initialDc, uint32_t& activeSlac, uint32_t& periods)
{
    if (configPath.empty()) { return; }
    std::ifstream in(configPath);
    if (!in) { return; }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    double value = 0.0;
#define APPLY_DOUBLE(jsonKey, member) if (ReadNumber(text, jsonKey, value)) { member = value; }
#define APPLY_UINT(jsonKey, member) if (ReadNumber(text, jsonKey, value)) { member = static_cast<uint32_t>(value); }
    APPLY_DOUBLE("t_ctrl_ms", params.m_tCtrlMs)
    APPLY_DOUBLE("slot_duration_us", params.m_slotDurationUs)
    APPLY_UINT("t_ctrl_slots", params.m_tCtrlSlots)
    APPLY_UINT("o_map_slots", params.m_oMapSlots)
    APPLY_UINT("c_req_eff_slots", params.m_cReqEffSlots)
    APPLY_UINT("c_res_eff_slots", params.m_cResEffSlots)
    APPLY_UINT("c_proc_slots", params.m_cProcSlots)
    APPLY_UINT("b_auth_slots", params.m_bAuthSlots)
    APPLY_UINT("b_pkt_slots", params.m_bPktSlots)
    APPLY_UINT("b_blk_slots", params.m_bBlkSlots)
    APPLY_UINT("c_slac_slots", params.m_cSlacSlots)
    APPLY_DOUBLE("d_slac_ms", params.m_dSlacMs)
    APPLY_UINT("initial_dc_evs", initialDc)
    APPLY_UINT("active_slac_sessions", activeSlac)
    APPLY_UINT("periods", periods)
#undef APPLY_DOUBLE
#undef APPLY_UINT
}

int
main(int argc, char* argv[])
{
    std::string config;
    std::string output = "results/ev-plc-transition";
    std::string experiment = "ev-plc-transition";
    std::string algorithm = "proposed_transition_aware";
    std::string ns3Command;
    std::string linkMix = "";
    uint32_t seed = 1;
    uint32_t bFix = 0;
    uint32_t initialDc = 12;
    uint32_t activeSlac = 2;
    uint32_t periods = 40;
    double degradationFactor = 1.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("config", "JSON config file", config);
    cmd.AddValue("output", "Output directory", output);
    cmd.AddValue("experiment", "Experiment name", experiment);
    cmd.AddValue("algorithm", "Algorithm label for scenario metadata", algorithm);
    cmd.AddValue("N0", "Initial DC-active EV count", initialDc);
    cmd.AddValue("slacBurst", "Active SLAC burst/session count", activeSlac);
    cmd.AddValue("seed", "Scenario seed", seed);
    cmd.AddValue("periods", "Number of periods to run", periods);
    cmd.AddValue("B_fix", "Fixed reservation budget metadata", bFix);
    cmd.AddValue("degradationFactor", "PLC degradation factor metadata", degradationFactor);
    cmd.AddValue("linkMix", "Optional PLC link mix metadata", linkMix);
    cmd.AddValue("ns3Command", "Full ns-3 command metadata", ns3Command);
    cmd.Parse(argc, argv);

    EvPlcParams params;
    ApplyConfig(config, params, initialDc, activeSlac, periods);
    if (degradationFactor != 1.0)
    {
        params.m_cReqEffSlots = static_cast<uint32_t>(std::ceil(params.m_cReqEffSlots * degradationFactor));
        params.m_cResEffSlots = static_cast<uint32_t>(std::ceil(params.m_cResEffSlots * degradationFactor));
    }

    EvPlcSimHelper helper;
    helper.Configure(params);
    helper.AddInitialDcEvs(initialDc);
    helper.ScheduleSlacArrivals(activeSlac);
    helper.Run(periods);
    helper.ExportMetrics(output, experiment, config, algorithm, seed, ns3Command);
    return 0;
}
