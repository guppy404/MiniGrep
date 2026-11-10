#pragma once

#include <vector>
#include <string>
#include "parser.hpp"

enum class StateType {
    Character,
    Wildcard,
    Split,
    Match
};

struct State {
    StateType type;
    char symbol = '\0';
    int out1 = -1;
    int out2 = -1;
};

struct NFA {
    std::vector<State> states;
    int start = -1;
};

NFA createLiteralNFA(char c);

NFA compileNFA(const std::vector<Token>& postfix);

bool fullMatch(const NFA& nfa, const std::string& text);

bool search(const NFA& nfa, const std::string& text);