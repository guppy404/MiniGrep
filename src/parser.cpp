#include "parser.hpp"
#include <stdexcept>

// convert to token representation
std::vector<Token> tokenize(const std::string& pattern) {
    std::vector<Token> tokens;

    for (char c : pattern) {
        switch (c) {
            case '(':
                tokens.push_back({TokenType::LeftParen});
                break;
            case ')':
                tokens.push_back({TokenType::RightParen});
                break;
            case '|':
                tokens.push_back({TokenType::Alt});
                break;
            case '*':
                tokens.push_back({TokenType::Star});
                break;
            case '.':
                tokens.push_back({TokenType::Wildcard});
                break;
            case '+':
                tokens.push_back({TokenType::Plus});
                break;
            case '?':
                tokens.push_back({TokenType::Question});
                break;
            default:
                tokens.push_back({TokenType::Literal, c});
                break;
        }
    }

    return tokens;
}

bool canEndExpression(TokenType type) {
    switch (type) {
        case TokenType::Literal:
        case TokenType::Wildcard:
        case TokenType::RightParen:
        case TokenType::Star:
        case TokenType::Plus:
        case TokenType::Question:
            return true;
        default:
            return false;
    }
}

bool canStartExpression(TokenType type) {
    switch (type) {
        case TokenType::Literal:
        case TokenType::Wildcard:
        case TokenType::LeftParen:
            return true;
        default:
            return false;
    }
}

// insert internal concat representation
std::vector<Token> insertConcatenation(
    const std::vector<Token>& tokens
) {
    std::vector<Token> result;

    for (size_t i = 0; i < tokens.size(); i++) {
        result.push_back(tokens[i]);

        if (i + 1 < tokens.size() && (canEndExpression(tokens[i].type) &&
            canStartExpression(tokens[i + 1].type))) {
            result.push_back({TokenType::Concat});
        }

    }
    return result;
}

int precedence(TokenType type) {
    switch (type) {
        case TokenType::Alt:
            return 1;
        case TokenType::Concat:
            return 2;
        case TokenType::Star:
            return 3;
        default:
            return 0;
    }
}

std::vector<Token> toPostfix(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    std::vector<Token> operators;

    for (const Token& token : tokens) {
        switch (token.type) {

            case TokenType::Literal:
            case TokenType::Wildcard:
                output.push_back(token);
                break;

            case TokenType::Star:
            case TokenType::Plus:
            case TokenType::Question:
                output.push_back(token);
                break;

            case TokenType::LeftParen:
                operators.push_back(token);
                break;

            case TokenType::RightParen:
                // pop operators until LeftParen then discard
                while (!operators.empty() &&
                    operators.back().type != TokenType::LeftParen) {

                    output.push_back(operators.back());
                    operators.pop_back();
                }
                if (operators.empty()) {
                    throw std::invalid_argument("Unmatched closing parenthesis");
                }
                operators.pop_back();
                break;

            case TokenType::Alt:
            case TokenType::Concat:
                // pop operators with >= precedence then push curr_op
                while (!operators.empty() &&
                    operators.back().type != TokenType::LeftParen &&
                    precedence(operators.back().type) >= precedence(token.type)) {

                    // move the top operator into output
                    output.push_back(operators.back());
                    operators.pop_back();
                }
                operators.push_back(token);
                break;
        }
    }

    // pop remaining operators
    while (!operators.empty()) {
        if (operators.back().type == TokenType::LeftParen) {
            throw std::invalid_argument("Unmatched opening parenthesis");
        }

        output.push_back(operators.back());
        operators.pop_back();
    }

    return output;
}

void validatePattern(const std::vector<Token>& tokens) {

    if (tokens.empty()) {
        return;
    }
    bool expectOperand = true;
    int depth = 0;

    for (std::size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];
        switch (token.type) {

            case TokenType::Literal:
            case TokenType::Wildcard:
                expectOperand = false;
                break;

            case TokenType::LeftParen:
                depth++;
                expectOperand = true;
                break;

            case TokenType::RightParen:
                if (depth == 0) {
                    throw std::invalid_argument("Unmatched closing parenthesis");
                }
                if (expectOperand) {
                    throw std::invalid_argument("Empty or incomplete group");
                }
                depth--;
                expectOperand = false;
                break;

            case TokenType::Star:
            case TokenType::Plus:
            case TokenType::Question: {

                if (expectOperand) {
                    throw std::invalid_argument(
                        "Quantifier has nothing to repeat"
                    );
                }

                if (i > 0) {
                    TokenType previous = tokens[i - 1].type;

                    if (previous == TokenType::Star ||
                        previous == TokenType::Plus ||
                        previous == TokenType::Question) {

                        throw std::invalid_argument(
                            "Consecutive quantifiers are not supported"
                        );
                    }
                }

                break;
            }

            case TokenType::Alt:
                if (expectOperand) {
                    throw std::invalid_argument("Missing left operand for |");
                }
                expectOperand = true;
                break;

            case TokenType::Concat:
                throw std::invalid_argument(
                    "Unexpected internal concatenation token"
                );
        }
    }

    if (depth != 0) {
        throw std::invalid_argument(
            "Unmatched ("
        );
    }

    if (expectOperand) {
        throw std::invalid_argument(
            "Incomplete final expression"
        );
    }
}