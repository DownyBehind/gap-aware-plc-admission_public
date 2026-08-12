#ifndef SLAC_SEQUENCE_MODEL_H
#define SLAC_SEQUENCE_MODEL_H
#include <cstdint>
#include <string>
#include <vector>
namespace ns3 {
enum class SlacMessageType { SLAC_PARM_REQ, SLAC_PARM_CNF, START_ATTEN_CHAR_IND, MNBC_SOUND_IND, ATTEN_CHAR_IND, ATTEN_CHAR_RSP, SLAC_MATCH_REQ, SLAC_MATCH_CNF };
struct SlacMessage { SlacMessageType type; uint32_t baseSlots{0}; bool uplink{false}; bool downlink{false}; };
std::string ToString(SlacMessageType type);
class SlacSequenceModel { public: static std::vector<SlacMessage> LegacySequence(); static uint32_t ComputeTotalSlots(const std::vector<SlacMessage>& sequence); static uint32_t CountMessages(const std::vector<SlacMessage>& sequence, SlacMessageType type); };
}
#endif
