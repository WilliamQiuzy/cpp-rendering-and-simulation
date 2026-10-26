#pragma once

// Minimal test harness: no dependencies, one executable per module, registered with CTest.

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace check {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

inline int run_all() {
    for (const TestCase& test : registry()) {
        const int before = failures();
        test.body();
        std::printf("[%s] %s\n", failures() == before ? "PASS" : "FAIL", test.name.c_str());
    }
    std::printf("%zu tests, %d failed checks\n", registry().size(), failures());
    return failures() == 0 ? 0 : 1;
}

}  // namespace check

#define CHECK_CONCAT_INNER(a, b) a##b
#define CHECK_CONCAT(a, b) CHECK_CONCAT_INNER(a, b)

#define TEST(name)                                                                    \
    static void name();                                                               \
    static check::Registrar CHECK_CONCAT(registrar_, name)(#name, name);              \
    static void name()

#define CHECK(condition)                                                              \
    do {                                                                              \
        if (!(condition)) {                                                           \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++check::failures();                                                      \
        }                                                                             \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                       \
    do {                                                                              \
        const double check_a = (actual), check_e = (expected);                        \
        if (!(std::abs(check_a - check_e) <= (tolerance))) {                          \
            std::printf("  %s:%d: %s = %.12g, expected %.12g (+/- %g)\n", __FILE__,   \
                        __LINE__, #actual, check_a, check_e, (double)(tolerance));    \
            ++check::failures();                                                      \
        }                                                                             \
    } while (0)
