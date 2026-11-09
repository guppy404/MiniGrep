#include <iostream>
#include <cassert>
#include <stdexcept>
#include "parser.hpp"

void printPostfix(const std::vector<Token>& tokens) {
    for (const Token& token : tokens) {
        switch (token.type) {
            case TokenType::Literal:
                std::cout << token.value;
                break;
            case TokenType::Wildcard:
                std::cout << '.';
                break;
            case TokenType::Concat:
                std::cout << '@';
                break;
            case TokenType::Alt:
                std::cout << '|';
                break;
            case TokenType::Star:
                std::cout << '*';
                break;
            default:
                break;
        }
    }
    std::cout << '\n';
}

void testValidation() {
    const std::vector<std::string> valid = {
        "",
        "a",
        "ab",
        "a|b",
        "(ab)*",
        "a(b|c)*d",
        "a.b",
        "((a))"
    };

    const std::vector<std::string> invalid = {
        "*a",
        "a**",
        "a|",
        "|a",
        "a||b",
        "()",
        "(a|)",
        "(ab",
        "ab)",
        "a(*b)"
    };

    for (const auto& pattern : valid) {
        validatePattern(tokenize(pattern));
    }

    for (const auto& pattern : invalid) {
        bool rejected = false;

        try {
            validatePattern(tokenize(pattern));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        assert(rejected && "Invalid pattern was accepted");
    }
}

int main() {
    std::cout << "Minigrep - custom regex engine\n";

    std::vector<std::string> patterns = {
        "ab",
        "a|b",
        "a*",
        "a(b|c)",
        "(ab)*",
        "a|bc*",
        "(a|b)*c",
        "a(b|c)*d"
    };

    for (const auto& pattern : patterns) {
        auto tokens = tokenize(pattern);
        auto explicitTokens = insertConcatenation(tokens);
        auto postfix = toPostfix(explicitTokens);

        std::cout << pattern << " -> ";
        printPostfix(postfix);
    }

    testValidation();

    return 0;
}