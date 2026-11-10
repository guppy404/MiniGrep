#!/usr/bin/env python3
"""Differential-test MiniGrep against Python's re.search."""

import random
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BINARY = ROOT / "minigrep"

RNG = random.Random(2026)
ALPHABET = "abcxyz012 "

def generate_pattern(depth=0):
    """Generate a pattern in MiniGrep's supported grammar."""
    if depth >= 3 or RNG.random() < 0.35:
        return RNG.choice(["a", "b", "c", "x", "."])

    operation = RNG.choice(["concat", "alt", "star", "plus", "optional"])

    if operation == "concat":
        return generate_pattern(depth + 1) + generate_pattern(depth + 1)

    if operation == "alt":
        left = generate_pattern(depth + 1)
        right = generate_pattern(depth + 1)
        return f"({left}|{right})"

    inner = generate_pattern(depth + 1)
    operator = {
        "star": "*",
        "plus": "+",
        "optional": "?"
    }[operation]

    return f"({inner}){operator}"


def generate_lines(count=100):
    lines = ["", "a", "b", "ab", "aaaa", "abc", "xyz",
             "aaab", "abab", "aabbc", "    "]

    for _ in range(count):
        length = RNG.randint(0, 30)
        lines.append(
            "".join(RNG.choices(ALPHABET, k=length))
        )

    return lines


def main():
    if not BINARY.exists():
        raise SystemExit("Build MiniGrep first: make")

    patterns = [
        "", "a", "ab", "a|b", "a*", "a+", "a?",
        "(ab)*", "(ab)+", "a?b", "(a|b)*c",
        "a.*b", "(a|aa)*b", "(a*)*",
        "((a|b)+c?)", "a|bc*"
    ]

    patterns += [generate_pattern() for _ in range(150)]
    lines = generate_lines()

    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "input.txt"
        path.write_text(
            "\n".join(lines) + "\n",
            encoding="ascii"
        )

        comparisons = 0

        for pattern in patterns:
            expected = [
                i for i, line in enumerate(lines, start=1)
                if re.search(pattern, line) is not None
            ]

            result = subprocess.run(
                [str(BINARY), "-n", pattern, str(path)],
                capture_output=True,
                text=True,
                check=False
            )

            if result.returncode not in (0, 1):
                raise AssertionError(
                    f"MiniGrep error on {pattern!r}: "
                    f"{result.stderr}"
                )

            actual = [
                int(row.split(":", 1)[0])
                for row in result.stdout.splitlines()
            ]

            if actual != expected:
                missing = sorted(set(expected) - set(actual))
                unexpected = sorted(set(actual) - set(expected))

                raise AssertionError(
                    f"Mismatch for {pattern!r}\n"
                    f"Missing lines: {missing}\n"
                    f"Unexpected lines: {unexpected}"
                )

            comparisons += len(lines)

    print(f"Patterns tested: {len(patterns)}")
    print(f"Lines per pattern: {len(lines)}")
    print(f"Comparisons passed: {comparisons}")
    print("Differential testing PASSED")


if __name__ == "__main__":
    main()
