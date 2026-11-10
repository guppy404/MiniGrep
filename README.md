# MiniGrep

A lightweight regular expression engine and grep-style command-line tool, built from scratch in **C++17** using **Thompson's NFA construction**.

MiniGrep parses regular expressions, compiles them into nondeterministic finite automata (NFAs), and simulates sets of active states to find matching text.

The matching engine is implemented without `std::regex`, PCRE, or third-party regex libraries.

## Features

- Custom regex tokenisation and syntax validation
- Infix-to-postfix conversion using the shunting-yard algorithm
- Thompson's NFA construction
- Epsilon-closure computation and NFA state-set simulation
- Full-string matching and substring search
- Line-by-line file searching
- Multiple input files and optional line numbers
- Automated unit, integration, and differential tests

## Supported Regex Syntax

| Syntax | Description | Example |
|--------|-------------|---------|
| `a` | Literal character | `abc` |
| `.` | Any single character | `a.c` |
| `ab` | Concatenation | `abc` |
| `a\|b` | Alternation | `cat\|dog` |
| `a*` | Zero or more repetitions | `ab*c` |
| `a+` | One or more repetitions | `ab+c` |
| `a?` | Zero or one occurrence | `colou?r` |
| `(ab)` | Grouping | `(ab)+c` |

Supported patterns operate on a basic byte-oriented character model.

Backreferences, lookaround, character classes, anchors, and escape sequences are not implemented.

## Build

Requirements:
- C++17-compatible compiler (GCC recommended)
- GNU Make
- Bash and Python 3 for the full test suite

```bash
git clone https://github.com/guppy404/minigrep.git
cd minigrep
make
```

## Usage

```bash
./minigrep PATTERN FILE
./minigrep -n PATTERN FILE
./minigrep PATTERN FILE1 FILE2
```

Search for errors or warnings:

```bash
./minigrep 'error|warning' logfile.txt
```

Display matching line numbers:

```bash
./minigrep -n 'error|warning' logfile.txt
```

Search with optional characters:

```bash
./minigrep 'colou?r' words.txt
```

Search for repeated groups:

```bash
./minigrep '(ab)+c' input.txt
```

For multiple files, matching output is prefixed with the filename.

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | At least one match found |
| 1 | No matches found |
| 2 | Invalid arguments, invalid pattern, or file error |

## Architecture

The engine consists of four stages:

```text
Regex pattern
     |
     v
Tokenisation and validation
     |
     v
Explicit concatenation + postfix conversion
     |
     v
Thompson NFA construction
     |
     v
NFA state-set simulation
     |
     v
Matching lines / full-match result
```

### 1. Parsing

Regex patterns are tokenised and validated. Implicit concatenation is made explicit, then the shunting-yard algorithm converts the pattern to postfix notation.

Example:

```text
Input:    a(b|c)*
Explicit: a@(b|c)*
Postfix:  abc|*@
```

Here `@` represents an internal concatenation operator.

### 2. Thompson's Construction

The compiler processes postfix tokens using a stack of NFA fragments.

Each fragment tracks:
- Its starting state
- A list of unresolved outgoing transitions

Fragments are combined using concatenation, alternation, and repetition operations.

Unresolved transitions are patched to their destinations as fragments are connected. A single accepting state is added when compilation finishes.

The NFA has four state types:

- `Character`: consumes a specific character
- `Wildcard`: consumes any character
- `Split`: follows epsilon transitions without consuming input
- `Match`: accepting state

States are stored in a `std::vector`, with transitions represented as integer indices rather than raw pointers.

### 3. NFA Simulation

The matcher maintains the set of currently reachable NFA states.

For each input character:

1. Examine active character-consuming states.
2. Follow matching transitions.
3. Compute epsilon closure.
4. Deduplicate reachable states.
5. Check acceptance when appropriate.

Substring searching additionally allows a new match to begin at each input position.

This avoids recursively exploring duplicate backtracking paths.

### 4. Command-Line Search

The CLI compiles the pattern once, then reads each input file line by line using `std::getline()`.

Each line is searched using the custom NFA engine, and matching lines are printed to stdout.

## Complexity

Let:
- `m` = number of NFA states
- `n` = length of the input string

Thompson NFA simulation:

- **Time:** O(nm)
- **Auxiliary simulation space:** O(m)

For a fixed regex, simulation time is linear in input length.

The implementation avoids the exponential path exploration possible in naive recursive backtracking regex engines.

The automaton itself uses O(m) storage. This analysis does not include input-line storage or the cost of parsing and constructing the automaton.

## Testing

Run all tests:

```bash
make test
```

Run memory-safety checks:

```bash
make sanitize
```

The test suite includes:

- 114 passing C++ unit checks
- 11 passing CLI integration tests
- 18,426 passing differential comparisons against Python's `re.search()`
- Edge cases involving empty strings, malformed patterns, grouping, and quantifiers

Differential testing uses a reproducible random seed and generates patterns restricted to the supported regex grammar.

Python's regex library is used only as a test oracle, never in the C++ implementation.

## Performance Benchmark

Run:

```bash
make bench
```

Benchmark configuration:

- Pattern: `(a|aa)*b`
- Input: `N` copies of `a`, followed by `c`
- 30 timed repetitions per input length
- Compiled with GCC, C++17, and `-O2`
- Measures full-string matching without regex compilation or file I/O

| Input length | Average time |
|--------------|--------------|
| 101 | 0.0096 ms |
| 501 | 0.0489 ms |
| 1,001 | 0.0981 ms |
| 2,001 | 0.2204 ms |
| 4,001 | 0.4279 ms |

These measurements show approximately linear scaling with input length for this fixed ambiguous regex on the tested system.

Results are machine-dependent. No performance comparison with production regex engines is claimed.

## Project Structure

```text
src/
  parser.hpp
  parser.cpp
  nfa.hpp
  nfa.cpp
  main.cpp

tests/
  test_regex.cpp
  test_cli.sh
  test_differential.py
  benchmark.cpp

Makefile
README.md
```

## Limitations

MiniGrep is an educational regex engine, not a fully compatible replacement for GNU grep.

Current limitations include:

- No character classes or ranges
- No anchors (`^`, `$`)
- No backreferences or capture extraction
- No lookahead or lookbehind
- No backslash escape syntax
- No full Unicode-aware matching
- No recursive directory search
- No DFA compilation or transition caching
- No full GNU grep CLI compatibility

Consecutive quantifiers such as `a**` and `a+?` are rejected.

## Possible Future Improvements

1. Lazy DFA construction and transition caching
2. Character classes, escapes, and anchors
3. Precomputed epsilon closures and reduced allocation overhead
4. Differential fuzzing with more complex patterns
5. Expanded CLI options and streaming input support

## References

- [Russ Cox — Regular Expression Matching Can Be Simple And Fast](https://swtch.com/~rsc/regexp/regexp1.html)
- [Build Your Own Regex Engines](https://build-your-own.org/b2a/r0_intro)
