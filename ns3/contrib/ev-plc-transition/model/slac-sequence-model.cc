// NOTE: this tier retains the superseded 20-message / 241-slot SLAC
// sequence. See docs/model/slac_sequence_model.md.
#include "slac-sequence-model.h"
namespace ns3 {
std::string ToString(SlacMessageType t){ switch(t){case SlacMessageType::SLAC_PARM_REQ:return "SLAC_PARM_REQ";case SlacMessageType::SLAC_PARM_CNF:return "SLAC_PARM_CNF";case SlacMessageType::START_ATTEN_CHAR_IND:return "START_ATTEN_CHAR_IND";case SlacMessageType::MNBC_SOUND_IND:return "MNBC_SOUND_IND";case SlacMessageType::ATTEN_CHAR_IND:return "ATTEN_CHAR_IND";case SlacMessageType::ATTEN_CHAR_RSP:return "ATTEN_CHAR_RSP";case SlacMessageType::SLAC_MATCH_REQ:return "SLAC_MATCH_REQ";case SlacMessageType::SLAC_MATCH_CNF:return "SLAC_MATCH_CNF";} return "UNKNOWN"; }
// Legacy 17-message sequence (196 slots), superseded by the
// 20-message sequence (241 slots). Not used on the paper's
// evaluation path; see docs/model/slac_sequence_model.md.
std::vector<SlacMessage> SlacSequenceModel::LegacySequence(){ std::vector<SlacMessage> s{{SlacMessageType::SLAC_PARM_REQ,11,true,false},{SlacMessageType::SLAC_PARM_CNF,11,false,true},{SlacMessageType::START_ATTEN_CHAR_IND,11,false,true}}; for(uint32_t i=0;i<10;++i) s.push_back({SlacMessageType::MNBC_SOUND_IND,11,true,false}); s.push_back({SlacMessageType::ATTEN_CHAR_IND,14,true,false}); s.push_back({SlacMessageType::ATTEN_CHAR_RSP,11,false,true}); s.push_back({SlacMessageType::SLAC_MATCH_REQ,14,true,false}); s.push_back({SlacMessageType::SLAC_MATCH_CNF,14,false,true}); return s; }
uint32_t SlacSequenceModel::ComputeTotalSlots(const std::vector<SlacMessage>& sequence){ uint32_t total=0; for(const auto& m:sequence) total+=m.baseSlots; return total; }
uint32_t SlacSequenceModel::CountMessages(const std::vector<SlacMessage>& sequence,SlacMessageType type){ uint32_t c=0; for(const auto& m:sequence) if(m.type==type) ++c; return c; }
}
