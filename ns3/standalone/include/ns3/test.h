#ifndef NS3_STANDALONE_TEST_H
#define NS3_STANDALONE_TEST_H

// Standalone stub for ns3/test.h: minimal TestCase/TestSuite
// with a global registry so the module's TestSuites run without ns-3.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ns3
{

class TestCase
{
  public:
    enum class Duration
    {
        QUICK,
        EXTENSIVE,
        TAKES_FOREVER
    };

    explicit TestCase(std::string name) : m_name(std::move(name)) {}
    virtual ~TestCase() = default;

    const std::string& GetName() const { return m_name; }
    bool Execute()
    {
        m_failed = false;
        DoRun();
        return !m_failed;
    }
    void ReportFailure(const std::string& message)
    {
        m_failed = true;
        std::cerr << "  FAIL [" << m_name << "] " << message << "\n";
    }

  private:
    virtual void DoRun() = 0;
    std::string m_name;
    bool m_failed{false};
};

class TestSuite
{
  public:
    enum class Type
    {
        UNIT,
        SYSTEM,
        PERFORMANCE,
        EXAMPLE
    };

    static std::vector<TestSuite*>& Registry()
    {
        static std::vector<TestSuite*> suites;
        return suites;
    }

    TestSuite(std::string name, Type /*type*/ = Type::UNIT) : m_name(std::move(name))
    {
        Registry().push_back(this);
    }
    virtual ~TestSuite()
    {
        for (auto* testCase : m_cases)
        {
            delete testCase;
        }
    }

    void AddTestCase(TestCase* testCase, TestCase::Duration /*duration*/ = TestCase::Duration::QUICK)
    {
        m_cases.push_back(testCase);
    }

    const std::string& GetName() const { return m_name; }
    bool RunSuite()
    {
        bool ok = true;
        for (auto* testCase : m_cases)
        {
            ok = testCase->Execute() && ok;
        }
        return ok;
    }

  private:
    std::string m_name;
    std::vector<TestCase*> m_cases;
};

inline int RunAllRegisteredTests()
{
    int failures = 0;
    for (auto* suite : TestSuite::Registry())
    {
        const bool ok = suite->RunSuite();
        std::cout << (ok ? "PASS" : "FAIL") << " TestSuite " << suite->GetName() << "\n";
        if (!ok)
        {
            ++failures;
        }
    }
    return failures;
}

} // namespace ns3

#define NS_TEST_ASSERT_MSG_EQ(actual, limit, msg)                                                  \
    do                                                                                             \
    {                                                                                              \
        if (!((actual) == (limit)))                                                                \
        {                                                                                          \
            std::ostringstream oss__;                                                              \
            oss__ << msg << " (got " << (actual) << ", expected " << (limit) << ")";               \
            this->ReportFailure(oss__.str());                                                      \
            return;                                                                                \
        }                                                                                          \
    } while (false)

#define NS_TEST_ASSERT_MSG_EQ_TOL(actual, limit, tol, msg)                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(((actual) >= (limit) - (tol)) && ((actual) <= (limit) + (tol))))                     \
        {                                                                                          \
            std::ostringstream oss__;                                                              \
            oss__ << msg << " (got " << (actual) << ", expected " << (limit) << " +/- " << (tol)   \
                  << ")";                                                                          \
            this->ReportFailure(oss__.str());                                                      \
            return;                                                                                \
        }                                                                                          \
    } while (false)

#define NS_TEST_ASSERT_MSG_GT_OR_EQ(actual, limit, msg)                                            \
    do                                                                                             \
    {                                                                                              \
        if (!((actual) >= (limit)))                                                                \
        {                                                                                          \
            std::ostringstream oss__;                                                              \
            oss__ << msg << " (got " << (actual) << ", expected >= " << (limit) << ")";            \
            this->ReportFailure(oss__.str());                                                      \
            return;                                                                                \
        }                                                                                          \
    } while (false)

#endif // NS3_STANDALONE_TEST_H
