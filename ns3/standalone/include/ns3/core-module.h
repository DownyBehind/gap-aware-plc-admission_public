#ifndef NS3_STANDALONE_CORE_MODULE_H
#define NS3_STANDALONE_CORE_MODULE_H

// Standalone stub for ns3/core-module.h: only CommandLine is
// consumed by the example driver.

#include "ns3/nstime.h"

#include <functional>
#include <map>
#include <sstream>
#include <string>

namespace ns3
{

class CommandLine
{
  public:
    explicit CommandLine(const char* /*file*/ = nullptr) {}

    template <typename T>
    void AddValue(const std::string& name, const std::string& /*help*/, T& value)
    {
        m_setters[name] = [&value](const std::string& text) {
            std::istringstream iss(text);
            iss >> value;
        };
    }

    void AddValue(const std::string& name, const std::string& /*help*/, std::string& value)
    {
        m_setters[name] = [&value](const std::string& text) { value = text; };
    }

    void Parse(int argc, char** argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string arg(argv[i]);
            if (arg.rfind("--", 0) != 0)
            {
                continue;
            }
            const auto eq = arg.find('=');
            if (eq == std::string::npos)
            {
                continue;
            }
            const std::string key = arg.substr(2, eq - 2);
            const std::string value = arg.substr(eq + 1);
            const auto it = m_setters.find(key);
            if (it != m_setters.end())
            {
                it->second(value);
            }
        }
    }

  private:
    std::map<std::string, std::function<void(const std::string&)>> m_setters;
};

} // namespace ns3

#endif // NS3_STANDALONE_CORE_MODULE_H
