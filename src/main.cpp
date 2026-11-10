#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "parser.hpp"
#include "nfa.hpp"

NFA compilePattern(const std::string& pattern) {
    auto tokens = tokenize(pattern);
    validatePattern(tokens);

    auto explicitTokens = insertConcatenation(tokens);
    auto postfix = toPostfix(explicitTokens);

    return compileNFA(postfix);
}

int main(int argc, char* argv[]) {
    bool showLineNumbers = false;
    int argIndex = 1;

    // Optional -n flag
    if (argIndex < argc &&
        std::string(argv[argIndex]) == "-n") {
        showLineNumbers = true;
        ++argIndex;
    }

    // Require a pattern and at least one file
    if (argc - argIndex < 2) {
        std::cerr
            << "Usage: minigrep [-n] PATTERN FILE [FILE...]\n";
        return 2;
    }

    std::string pattern = argv[argIndex++];
    std::vector<std::string> filenames;

    for (int i = argIndex; i < argc; ++i) {
        filenames.emplace_back(argv[i]);
    }

    NFA nfa;

    try {
        nfa = compilePattern(pattern);
    } catch (const std::invalid_argument& e) {
        std::cerr << "minigrep: invalid pattern: "
                  << e.what() << '\n';
        return 2;
    }

    bool anyMatch = false;
    bool fileError = false;
    bool multipleFiles = filenames.size() > 1;

    for (const std::string& filename : filenames) {
        std::ifstream file(filename);

        if (!file) {
            std::cerr << "minigrep: cannot open "
                      << filename << '\n';
            fileError = true;
            continue;
        }

        std::string line;
        std::size_t lineNumber = 0;

        while (std::getline(file, line)) {
            ++lineNumber;

            if (!search(nfa, line)) {
                continue;
            }

            anyMatch = true;

            if (multipleFiles) {
                std::cout << filename << ':';
            }

            if (showLineNumbers) {
                std::cout << lineNumber << ':';
            }

            std::cout << line << '\n';
        }

        if (file.bad()) {
            std::cerr << "minigrep: read error: "
                      << filename << '\n';
            fileError = true;
        }
    }

    if (fileError) {
        return 2;
    }

    return anyMatch ? 0 : 1;
}
