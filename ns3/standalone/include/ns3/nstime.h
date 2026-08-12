#ifndef NS3_STANDALONE_NSTIME_H
#define NS3_STANDALONE_NSTIME_H

// Standalone stub for ns3/nstime.h: the module only uses
// Time as a microsecond value carrier. Numbers produced under this stub
// are labeled "slot-accurate simulator" until reproduced under a real
// ns-3 build (track B).

namespace ns3
{

class Time
{
  public:
    Time() = default;
    explicit Time(double us) : m_us(us) {}
    double GetMicroSeconds() const { return m_us; }
    double GetMilliSeconds() const { return m_us / 1e3; }
    double GetSeconds() const { return m_us / 1e6; }
    Time operator+(const Time& other) const { return Time(m_us + other.m_us); }
    Time operator-(const Time& other) const { return Time(m_us - other.m_us); }
    bool operator<(const Time& other) const { return m_us < other.m_us; }
    bool operator<=(const Time& other) const { return m_us <= other.m_us; }
    bool operator>(const Time& other) const { return m_us > other.m_us; }
    bool operator>=(const Time& other) const { return m_us >= other.m_us; }
    bool operator==(const Time& other) const { return m_us == other.m_us; }

  private:
    double m_us{0.0};
};

inline Time MicroSeconds(double us) { return Time(us); }
inline Time MilliSeconds(double ms) { return Time(ms * 1e3); }
inline Time Seconds(double s) { return Time(s * 1e6); }

} // namespace ns3

#endif // NS3_STANDALONE_NSTIME_H
