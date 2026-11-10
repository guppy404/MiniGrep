#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/minigrep"

TMPDIR_TEST="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_TEST"' EXIT

cat > "$TMPDIR_TEST/first.txt" <<'EOF'
Everything is working
An error occurred
Connection successful
A warning was generated
No issues detected
Another error in the system
EOF

cat > "$TMPDIR_TEST/second.txt" <<'EOF'
hello world
error on line two
warning on line three
EOF

passed=0
failed=0

run_test() {
    local name="$1"
    local expected_status="$2"
    local expected_output="$3"
    shift 3

    local actual_output
    local actual_status

    if actual_output="$("$BIN" "$@" 2>/dev/null)"; then
        actual_status=0
    else
        actual_status=$?
    fi

    if [[ "$actual_status" -eq "$expected_status" &&
          "$actual_output" == "$expected_output" ]]; then
        echo "[PASS] $name"
        ((passed += 1))
    else
        echo "[FAIL] $name"
        echo "  Expected status: $expected_status"
        echo "  Actual status:   $actual_status"
        printf '  Expected output: %q\n' "$expected_output"
        printf '  Actual output:   %q\n' "$actual_output"
        ((failed += 1))
    fi
}

run_test "Basic substring search" 0 \
    "An error occurred
Another error in the system" \
    'error' "$TMPDIR_TEST/first.txt"

run_test "Alternation" 0 \
    "An error occurred
A warning was generated
Another error in the system" \
    'error|warning' "$TMPDIR_TEST/first.txt"

run_test "Line numbers" 0 \
    "2:An error occurred
4:A warning was generated
6:Another error in the system" \
    -n 'error|warning' "$TMPDIR_TEST/first.txt"

run_test "No matches" 1 "" \
    'notfound' "$TMPDIR_TEST/first.txt"

run_test "Invalid regex" 2 "" \
    'a||b' "$TMPDIR_TEST/first.txt"

run_test "Missing file" 2 "" \
    'error' "$TMPDIR_TEST/missing.txt"

run_test "Multiple files" 0 \
    "$TMPDIR_TEST/first.txt:An error occurred
$TMPDIR_TEST/first.txt:Another error in the system
$TMPDIR_TEST/second.txt:error on line two" \
    'error' \
    "$TMPDIR_TEST/first.txt" \
    "$TMPDIR_TEST/second.txt"

run_test "Multiple files with line numbers" 0 \
    "$TMPDIR_TEST/first.txt:2:An error occurred
$TMPDIR_TEST/first.txt:6:Another error in the system
$TMPDIR_TEST/second.txt:2:error on line two" \
    -n 'error' \
    "$TMPDIR_TEST/first.txt" \
    "$TMPDIR_TEST/second.txt"

run_test "Empty regex" 0 \
    "Everything is working
An error occurred
Connection successful
A warning was generated
No issues detected
Another error in the system" \
    '' "$TMPDIR_TEST/first.txt"

run_test "Empty pattern in missing file" 2 "" \
    '' "$TMPDIR_TEST/missing.txt"

run_test "No command arguments" 2 "" 

echo
echo "CLI tests passed: $passed"
echo "CLI tests failed: $failed"

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi
