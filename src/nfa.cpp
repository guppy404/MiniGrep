#include "nfa.hpp"

#include <stdexcept>
#include <utility>

struct Patch {
    int state;
    bool second;  // false = out1, true = out2
};

struct Fragment {
    int start;
    std::vector<Patch> outs;
};

NFA createLiteralNFA(char c) {
    NFA nfa;

    nfa.states.push_back({
        StateType::Character,
        c
    });
    nfa.states.push_back({
        StateType::Match,
    });

    nfa.states[0].out1 = 1;
    nfa.start = 0;
    return nfa;
}

static int addState(NFA& nfa, State state) {
    int index = static_cast<int>(nfa.states.size());
    nfa.states.push_back(state);
    return index;
}

static void patch(NFA& nfa,
           const std::vector<Patch>& outs,
           int target) {
    for (const Patch& p : outs) {
        if (p.second) {
            nfa.states[p.state].out2 = target;
        } else {
            nfa.states[p.state].out1 = target;
        }
    }
}

static Fragment popFragment(std::vector<Fragment>& stack) {
    if (stack.empty()) {
        throw std::invalid_argument(
            "Not enough operands in postfix expression"
        );
    }

    Fragment fragment = std::move(stack.back());
    stack.pop_back();
    return fragment;
}

NFA compileNFA(const std::vector<Token>& postfix) {
    NFA nfa;
    std::vector<Fragment> stack;

    // Empty regex matches only the empty string.
    if (postfix.empty()) {
        nfa.start = addState(nfa, {StateType::Match});
        return nfa;
    }

    for (const Token& token : postfix) {
        switch (token.type) {

            case TokenType::Literal:
            case TokenType::Wildcard: {
                StateType type = token.type == TokenType::Literal
                    ? StateType::Character : StateType::Wildcard;

                int index = addState(nfa, {type, token.value});
                stack.push_back({
                    index,
                    {{index, false}}
                });

                break;
            }

            case TokenType::Concat: {

                Fragment b = popFragment(stack);
                Fragment a = popFragment(stack);

                patch(nfa, a.outs, b.start);

                stack.push_back({
                    a.start,
                    std::move(b.outs)
                });

                break;
            }

            case TokenType::Alt: {
                
                Fragment b = popFragment(stack);
                Fragment a = popFragment(stack);

                int split = addState(nfa, {
                    StateType::Split,
                    '\0',
                    a.start,
                    b.start
                });

                std::vector<Patch> outs = std::move(a.outs);
                outs.insert(
                    outs.end(),
                    b.outs.begin(),
                    b.outs.end()
                );

                stack.push_back({
                    split,
                    std::move(outs)
                });

                break;
            }

            case TokenType::Star: {

                Fragment a = popFragment(stack);
                int split = addState(nfa, {
                    StateType::Split,
                    '\0',
                    a.start,
                    -1
                });
                patch(nfa, a.outs, split);
                stack.push_back({
                    split,
                    {{split, true}}
                });
                break;
            }

            case TokenType::Plus: {
                Fragment a = popFragment(stack);

                int split = addState(nfa, {
                    StateType::Split,
                    '\0',
                    a.start,
                    -1
                });

                // Reconnect A's unfinished transitions
                // to the Split state.
                patch(nfa, a.outs, split);

                // Crucial difference from Star:
                // start at A, not the Split.
                stack.push_back({
                    a.start,
                    {{split, true}}
                });

                break;
            }

            case TokenType::Question: {
                Fragment a = popFragment(stack);

                int split = addState(nfa, {
                    StateType::Split,
                    '\0',
                    a.start,
                    -1
                });

                // Preserve A's unfinished transitions.
                std::vector<Patch> outs = std::move(a.outs);

                // Also remember Split.out2 (the skip path).
                outs.push_back({split, true});

                stack.push_back({
                    split,
                    std::move(outs)
                });

                break;
            }

            default:
                throw std::invalid_argument(
                    "Unexpected token in postfix expression"
                );
        }
    }

    if (stack.size() != 1) {
        throw std::invalid_argument(
            "Invalid postfix expression"
        );
    }

    Fragment result = popFragment(stack);
    int matchIndex = addState(nfa, {StateType::Match});
    patch(nfa, result.outs, matchIndex);
    nfa.start = result.start;

    return nfa;
}

static void addStateClosure(
    const NFA& nfa,
    int start,
    std::vector<int>& active,
    std::vector<bool>& visited
) {
    std::vector<int> stack = {start};

    while (!stack.empty()) {
        int index = stack.back();
        stack.pop_back();

        if (index == -1 || visited[index]) {
            continue;
        }

        visited[index] = true;

        const State& state = nfa.states[index];

        if (state.type == StateType::Split) {
            if (state.out1 != -1) {
                stack.push_back(state.out1);
            }
            if (state.out2 != -1) {
                stack.push_back(state.out2);
            }
        } else {
            active.push_back(index);
        }
    }
}

static bool containsMatch(
    const NFA& nfa,
    const std::vector<int>& active
) {
    for (int index : active) {
        if (nfa.states[index].type == StateType::Match) {
            return true;
        }
    }
    return false;
}

bool fullMatch(const NFA& nfa, const std::string& text) {
    std::vector<int> current;
    std::vector<bool> visited(nfa.states.size(), false);

    // Start by following all epsilon transitions.
    addStateClosure(nfa, nfa.start, current, visited);

    for (char c : text) {
        std::vector<int> next;
        std::vector<bool> nextVisited(nfa.states.size(), false);

        for (int index : current) {
            const State& state = nfa.states[index];

            bool matches =
                (state.type == StateType::Character &&
                 state.symbol == c) ||
                (state.type == StateType::Wildcard);

            if (matches) {
                addStateClosure(
                    nfa,
                    state.out1,
                    next,
                    nextVisited
                );
            }
        }

        current = std::move(next);

        if (current.empty()) {
            return false;
        }
    }

    // After consuming all input, an accepting state
    // must be present in the active set.
    return containsMatch(nfa, current);
}


bool search(const NFA& nfa, const std::string& text) {
    std::vector<int> current;
    std::vector<bool> visited(nfa.states.size(), false);

    addStateClosure(nfa, nfa.start, current, visited);

    // Check for a zero-length match.
    if (containsMatch(nfa, current)) {
        return true;
    }

    for (char c : text) {
        // Start a new possible match at this position.
        // Use the existing visited set to avoid duplicates.
        addStateClosure(nfa, nfa.start, current, visited);

        std::vector<int> next;
        std::vector<bool> nextVisited(nfa.states.size(), false);

        for (int index : current) {
            const State& state = nfa.states[index];

            bool matches =
                (state.type == StateType::Character &&
                 state.symbol == c) ||
                state.type == StateType::Wildcard;

            if (matches) {
                addStateClosure(
                    nfa,
                    state.out1,
                    next,
                    nextVisited
                );
            }
        }

        if (containsMatch(nfa, next)) {
            return true;
        }

        current = std::move(next);
        visited = std::move(nextVisited);
    }

    return false;
}