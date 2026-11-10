#pragma once

#include <string>
#include <vector>

enum class TokenType {
    Literal,
    Wildcard,
    Concat,
    Alt,
    Star,
    Plus,
    Question,
    LeftParen,
    RightParen
};

struct Token {
    TokenType type;
    char value = '\0';
};

std::vector<Token> tokenize(const std::string& pattern);

std::vector<Token> insertConcatenation(
    const std::vector<Token>& tokens
);

std::vector<Token> toPostfix(const std::vector<Token>& tokens);

void validatePattern(const std::vector<Token>& tokens);