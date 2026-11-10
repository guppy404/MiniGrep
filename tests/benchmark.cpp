#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "parser.hpp"
#include "nfa.hpp"

NFA compilePattern(const std::string& pattern) {
    auto tokens = tokenize(pattern);
    validatePattern(tokens);
    return compileNFA(toPostfix(insertConcatenation(tokens)));
}

int main() {
    const NFA nfa = compilePattern("(a|aa)*b");

    const std::vector<int> lengths = {
        100, 500, 1000, 2000, 4000
    };

    constexpr int repetitions = 30;

    std::cout << "Pattern: (a|aa)*b\n";
    std::cout << "Input: N copies of 'a', followed by 'c'\n";
    std::cout << "Repetitions: " << repetitions << "\n\n";

    std::cout << std::left << std::setw(12) << "Length"
              << std::setw(18) << "Average (ms)"
              << "Result\n";

    for (int n : lengths) {
        const std::string input(n, 'a');
        const std::string text = input + "c";

        // Warm up the matcher.
        for (int i = 0; i < 5; ++i) {
            fullMatch(nfa, text);
        }

        auto start = std::chrono::steady_clock::now();

        bool matched = false;

        for (int i = 0; i < repetitions; ++i) {
            matched = fullMatch(nfa, text);
        }

        auto end = std::chrono::steady_clock::now();

        if (matched) {
            std::cerr << "Unexpected match\n";
            return 1;
        }

        double elapsedMs =
            std::chrono::duration<double, std::milli>(
                end - start
            ).count();

        std::cout << std::left << std::setw(12)
                  << text.size()
                  << std::setw(18) << std::fixed
                  << std::setprecision(4)
                  << elapsedMs / repetitions
                  << "No match\n";
    }
}
