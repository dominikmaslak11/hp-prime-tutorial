#pragma once

// Minimal test harness (no external dependencies).

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace testing {

struct Case {
    const char *name;
    std::function<void()> fn;
};

inline std::vector<Case> &registry()
{
    static std::vector<Case> r;
    return r;
}

inline int &failures()
{
    static int f = 0;
    return f;
}

struct Registrar {
    Registrar(const char *name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

inline void fail(const char *file, int line, const std::string &msg)
{
    ++failures();
    std::printf("  FAIL %s:%d: %s\n", file, line, msg.c_str());
}

inline int runAll()
{
    int failedCases = 0;
    for (const auto &c : registry()) {
        int before = failures();
        c.fn();
        bool ok = failures() == before;
        if (!ok)
            ++failedCases;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
    }
    std::printf("\n%zu testów, %d nieudanych\n", registry().size(), failedCases);
    return failedCases == 0 ? 0 : 1;
}

} // namespace testing

#define TEST_CAT2(a, b) a##b
#define TEST_CAT(a, b) TEST_CAT2(a, b)
#define TEST(name)                                                                                                     \
    static void TEST_CAT(test_, name)();                                                                               \
    static testing::Registrar TEST_CAT(reg_, name)(#name, TEST_CAT(test_, name));                                      \
    static void TEST_CAT(test_, name)()

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            testing::fail(__FILE__, __LINE__, #cond);                                                                  \
    } while (0)

#define CHECK_MSG(cond, msg)                                                                                           \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            testing::fail(__FILE__, __LINE__, std::string(#cond) + " — " + (msg));                                    \
    } while (0)
