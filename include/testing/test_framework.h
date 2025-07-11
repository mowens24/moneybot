#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <iostream>
#include <sstream>

namespace moneybot {
namespace testing {

class TestResult {
public:
    bool passed;
    std::string message;
    std::string test_name;
    std::chrono::milliseconds duration;
    
    TestResult(bool p, const std::string& msg, const std::string& name, 
               std::chrono::milliseconds dur = std::chrono::milliseconds(0))
        : passed(p), message(msg), test_name(name), duration(dur) {}
};

class TestSuite {
public:
    using TestFunction = std::function<void()>;
    
    TestSuite(const std::string& name) : suite_name_(name) {}
    
    void addTest(const std::string& test_name, TestFunction test_func) {
        tests_.push_back({test_name, test_func});
    }
    
    std::vector<TestResult> run() {
        std::vector<TestResult> results;
        
        std::cout << "\n=== Running Test Suite: " << suite_name_ << " ===" << std::endl;
        
        for (const auto& test : tests_) {
            auto start = std::chrono::high_resolution_clock::now();
            
            try {
                test.second(); // Run test function
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                results.emplace_back(true, "PASSED", test.first, duration);
                std::cout << "  ✓ " << test.first << " (" << duration.count() << "ms)" << std::endl;
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                results.emplace_back(false, e.what(), test.first, duration);
                std::cout << "  ✗ " << test.first << " - " << e.what() << " (" << duration.count() << "ms)" << std::endl;
            } catch (...) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                results.emplace_back(false, "Unknown exception", test.first, duration);
                std::cout << "  ✗ " << test.first << " - Unknown exception (" << duration.count() << "ms)" << std::endl;
            }
        }
        
        return results;
    }
    
    const std::string& getName() const { return suite_name_; }

private:
    std::string suite_name_;
    std::vector<std::pair<std::string, TestFunction>> tests_;
};

class TestRunner {
public:
    void addSuite(std::shared_ptr<TestSuite> suite) {
        suites_.push_back(suite);
    }
    
    void runAll() {
        int total_tests = 0;
        int passed_tests = 0;
        auto overall_start = std::chrono::high_resolution_clock::now();
        
        std::cout << "\n════════════════════════════════════════" << std::endl;
        std::cout << "         MoneyBot Test Runner" << std::endl;
        std::cout << "════════════════════════════════════════" << std::endl;
        
        for (auto& suite : suites_) {
            auto results = suite->run();
            
            for (const auto& result : results) {
                total_tests++;
                if (result.passed) {
                    passed_tests++;
                }
            }
        }
        
        auto overall_end = std::chrono::high_resolution_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start);
        
        std::cout << "\n════════════════════════════════════════" << std::endl;
        std::cout << "Test Results:" << std::endl;
        std::cout << "  Total Tests: " << total_tests << std::endl;
        std::cout << "  Passed: " << passed_tests << std::endl;
        std::cout << "  Failed: " << (total_tests - passed_tests) << std::endl;
        std::cout << "  Duration: " << overall_duration.count() << "ms" << std::endl;
        
        if (passed_tests == total_tests) {
            std::cout << "  Result: ✓ ALL TESTS PASSED" << std::endl;
        } else {
            std::cout << "  Result: ✗ SOME TESTS FAILED" << std::endl;
        }
        std::cout << "════════════════════════════════════════" << std::endl;
    }

private:
    std::vector<std::shared_ptr<TestSuite>> suites_;
};

// Test assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::stringstream ss; \
        ss << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_FALSE(condition) \
    if ((condition)) { \
        std::stringstream ss; \
        ss << "Assertion failed: " << #condition << " should be false at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::stringstream ss; \
        ss << "Assertion failed: Expected " << (expected) << " but got " << (actual) << " at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_NE(expected, actual) \
    if ((expected) == (actual)) { \
        std::stringstream ss; \
        ss << "Assertion failed: Expected " << (expected) << " to not equal " << (actual) << " at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_NEAR(expected, actual, tolerance) \
    if (std::abs((expected) - (actual)) > (tolerance)) { \
        std::stringstream ss; \
        ss << "Assertion failed: Expected " << (expected) << " ± " << (tolerance) << " but got " << (actual) << " at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_THROWS(expression, exception_type) \
    try { \
        expression; \
        std::stringstream ss; \
        ss << "Assertion failed: Expected " << #exception_type << " to be thrown at " << __FILE__ << ":" << __LINE__; \
        throw std::runtime_error(ss.str()); \
    } catch (const exception_type&) { \
        /* Expected exception caught */ \
    }

} // namespace testing
} // namespace moneybot

#endif // TEST_FRAMEWORK_H
