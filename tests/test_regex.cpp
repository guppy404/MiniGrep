#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "parser.hpp"
#include "nfa.hpp"

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& description) {
    if (condition) {
        ++passed;
    } else {
        ++failed;
        std::cerr << "[FAIL] " << description << '\n';
    }
}

// Use the same compilation pipeline as the CLI.
NFA compilePattern(const std::string& pattern) {
    auto tokens = tokenize(pattern);
    validatePattern(tokens);
    auto explicitTokens = insertConcatenation(tokens);
    auto postfix = toPostfix(explicitTokens);
    return compileNFA(postfix);
}

std::string tokensToString(const std::vector<Token>& tokens) {
    std::string result;

    for (const Token& token : tokens) {
        switch (token.type) {
            case TokenType::Literal:
                result += token.value;
                break;
            case TokenType::Wildcard:
                result += '.';
                break;
            case TokenType::Concat:
                result += '@';
                break;
            case TokenType::Alt:
                result += '|';
                break;
            case TokenType::Star:
                result += '*';
                break;
            case TokenType::Plus:
                result += '+';
                break;
            case TokenType::Question:
                result += '?';
                break;
            case TokenType::LeftParen:
                result += '(';
                break;
            case TokenType::RightParen:
                result += ')';
                break;
        }
    }

    return result;
}

void testParsing() {
    struct Case {
        std::string pattern;
        std::string expected;
    };

    const std::vector<Case> cases = {
        {"", ""},
        {"a", "a"},
        {"ab", "ab@"},
        {"a|b", "ab|"},
        {"a*", "a*"},
        {"a+", "a+"},
        {"a?", "a?"},
        {"(ab)*", "ab@*"},
        {"a(b|c)", "abc|@"},
        {"a|bc*", "abc*@|"},
        {"(a|b)*c", "ab|*c@"},
        {"a(b|c)*d", "abc|*@d@"},
        {"a?b+", "a?b+@"},
        {"(ab)+c?", "ab@+c?@"},
        {"a.b", "a.@b@"}
    };

    for (const auto& tc : cases) {
        auto tokens = tokenize(tc.pattern);
        validatePattern(tokens);

        auto explicitTokens = insertConcatenation(tokens);
        auto postfix = toPostfix(explicitTokens);

        check(
            tokensToString(postfix) == tc.expected,
            "Postfix: " + tc.pattern
        );
    }
}

void testInvalidPatterns() {
    const std::vector<std::string> invalid = {
        "*a", "+a", "?a",
        "a|", "|a", "a||b",
        "()", "(a|)", "(|a)",
        "(ab", "ab)", "a(*b)",
        "a**", "a++", "a??",
        "a?*", "a*+", "a+?",
        "a|*b"
    };

    for (const auto& pattern : invalid) {
        bool rejected = false;

        try {
            compilePattern(pattern);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        check(rejected, "Reject invalid pattern: " + pattern);
    }
}

void testNFAConstruction() {
    {
        NFA nfa = compilePattern("ab");

        check(nfa.states.size() == 3, "ab: three states");
        check(nfa.start == 0, "ab: correct start");
        check(nfa.states[0].out1 == 1, "ab: a connects to b");
        check(nfa.states[1].out1 == 2, "ab: b connects to Match");
        check(nfa.states[2].type == StateType::Match,
              "ab: accepting state");
    }

    {
        NFA nfa = compilePattern("a|b");

        check(nfa.states.size() == 4, "a|b: four states");
        check(nfa.states[nfa.start].type == StateType::Split,
              "a|b: start is Split");
        check(nfa.states[nfa.start].out1 == 0,
              "a|b: first branch");
        check(nfa.states[nfa.start].out2 == 1,
              "a|b: second branch");
    }

    {
        NFA nfa = compilePattern("a*");

        check(nfa.states.size() == 3, "a*: three states");
        check(nfa.states[nfa.start].type == StateType::Split,
              "a*: starts with Split");
        check(nfa.states[0].out1 == nfa.start,
              "a*: repetition loops back");
    }

    {
        NFA nfa = compilePattern("");

        check(nfa.states.size() == 1, "Empty regex: one state");
        check(nfa.states[nfa.start].type == StateType::Match,
              "Empty regex: immediate acceptance");
    }
}

void testFullMatching() {
    struct Case {
        std::string pattern;
        std::string text;
        bool expected;
    };

    const std::vector<Case> cases = {
        {"", "", true},
        {"", "a", false},
        {"a", "a", true},
        {"a", "", false},
        {"ab", "ab", true},
        {"ab", "a", false},
        {"ab", "abc", false},
        {"a|b", "a", true},
        {"a|b", "b", true},
        {"a|b", "c", false},
        {"a*", "", true},
        {"a*", "aaaa", true},
        {"a*", "aaab", false},
        {"a+", "", false},
        {"a+", "a", true},
        {"a+", "aaaa", true},
        {"a?", "", true},
        {"a?", "a", true},
        {"a?", "aa", false},
        {"(ab)+", "abab", true},
        {"(ab)+", "aba", false},
        {"a?b", "b", true},
        {"a?b", "ab", true},
        {"a?b", "aab", false},
        {"(a|b)*c", "c", true},
        {"(a|b)*c", "abababc", true},
        {"(a|b)*c", "ababab", false},
        {"a.b", "acb", true},
        {"a.b", "ab", false},
        {"a.*b", "aHELLO7b", true},
        {"a.*b", "ac", false},
        {"(a*)*", "", true},
        {"(a*)*", "aaaa", true},
        {"(a|aa)*b", "aaaaab", true},
        {"(a|aa)*b", "aaaaac", false},
        {"a|bc", "bc", true},
        {"a|bc", "ac", false},
        {"(a|b)c", "ac", true},
        {"(a|b)c", "bc", true}
    };

    for (const auto& tc : cases) {
        NFA nfa = compilePattern(tc.pattern);

        bool actual = fullMatch(nfa, tc.text);

        check(
            actual == tc.expected,
            "Full match: /" + tc.pattern +
            "/ against \"" + tc.text + "\""
        );
    }
}

void testSubstringSearch() {
    struct Case {
        std::string pattern;
        std::string text;
        bool expected;
    };

    const std::vector<Case> cases = {
        {"abc", "abc", true},
        {"abc", "xyzabc", true},
        {"abc", "abcdef", true},
        {"abc", "xyzabcdef", true},
        {"abc", "xyz", false},
        {"abc", "acb", false},
        {"a|b", "xyzbxyz", true},
        {"a|b", "xyz", false},
        {"a*b", "xxaaabyy", true},
        {"a*b", "xxbyy", true},
        {"a*b", "xxxx", false},
        {"a.*b", "xxaHELLObyy", true},
        {"a.*b", "xxaHELLOByy", false},
        {"a+", "xxaaay", true},
        {"a+", "xyz", false},
        {"a?b", "xxbyy", true},
        {"a?b", "xxabyy", true},
        {"a?b", "xxxx", false},
        {"", "", true},
        {"", "anything", true},
        {"a*", "xyz", true},
        {"a+", "", false},
        {"(ab)+c", "xxababcxx", true},
        {"(ab)+c", "xxabax", false},
        {"z", "abcxyz", true},
        {"z", "abcxy", false},
        {"abc", "xxab", false}
    };

    for (const auto& tc : cases) {
        NFA nfa = compilePattern(tc.pattern);

        check(
            search(nfa, tc.text) == tc.expected,
            "Search: /" + tc.pattern +
            "/ in \"" + tc.text + "\""
        );
    }
}

int main() {
    try {
        testParsing();
        testInvalidPatterns();
        testNFAConstruction();
        testFullMatching();
        testSubstringSearch();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Unhandled exception: "
                  << e.what() << '\n';
        return 1;
    }

    std::cout << "\nTests passed: " << passed << '\n';
    std::cout << "Tests failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
