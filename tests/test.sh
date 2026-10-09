#!/bin/sh
# ==============================================================================
# tests/test.sh - Test suite for NetBSD 10.1 ls(1) midterm project
# Compatible with NetBSD /bin/sh and standard POSIX shells
# ==============================================================================

MYLS="./myls"
SYSLS="/bin/ls"
PASSED=0
FAILED=0
TOTAL=0
SANDBOX="tests/sandbox"

# Detect color support
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    GREEN=''
    RED=''
    YELLOW=''
    BLUE=''
    NC=''
fi

if [ ! -x "$MYLS" ]; then
    echo "Error: $MYLS not found. Please run make first."
    exit 1
fi

setup_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX/dir1/subdir1"
    mkdir -p "$SANDBOX/dir2"
    mkdir -p "$SANDBOX/empty_dir"
    mkdir -p "$SANDBOX/.hidden_dir"

    echo "Small file" > "$SANDBOX/file_a.txt"
    touch -t 202101010000 "$SANDBOX/file_a.txt" 2>/dev/null || true

    echo "Medium file content for testing file size sorting" > "$SANDBOX/file_b.txt"
    touch -t 202306011200 "$SANDBOX/file_b.txt" 2>/dev/null || true

    echo "Large file payload with plenty of text data to ensure size sorting works properly across different block sizes" > "$SANDBOX/file_c.txt"
    touch -t 202409010830 "$SANDBOX/file_c.txt" 2>/dev/null || true

    echo "Hidden content" > "$SANDBOX/.hidden_file"
    echo "Sub content" > "$SANDBOX/dir1/subdir1/nested.txt"

    echo '#!/bin/sh' > "$SANDBOX/script.sh"
    chmod +x "$SANDBOX/script.sh"

    ln -s "file_a.txt" "$SANDBOX/symlink_ok" 2>/dev/null || true
    ln -s "non_existent_target" "$SANDBOX/dangling_symlink" 2>/dev/null || true
    ln -s "dir1" "$SANDBOX/symlink_dir" 2>/dev/null || true
}

cleanup_sandbox() {
    rm -rf "$SANDBOX"
    rm -f /tmp/sysls.out /tmp/sysls.err /tmp/myls.out /tmp/myls.err
}

run_test() {
    desc="$1"
    flags="$2"
    shift 2
    args="$@"

    TOTAL=$((TOTAL + 1))
    printf "%-30s ... " "$desc"

    # Run system ls
    LC_ALL=C $SYSLS $flags $args > /tmp/sysls.out 2> /tmp/sysls.err
    sys_rc=$?

    # Run myls
    LC_ALL=C $MYLS $flags $args > /tmp/myls.out 2> /tmp/myls.err
    my_rc=$?

    diff_out=$(diff -u /tmp/sysls.out /tmp/myls.out 2>&1)
    diff_rc=$?

    rc_ok=0
    if [ "$sys_rc" -eq 0 ] && [ "$my_rc" -eq 0 ]; then
        rc_ok=1
    elif [ "$sys_rc" -ne 0 ] && [ "$my_rc" -ne 0 ]; then
        rc_ok=1
    fi

    if [ "$diff_rc" -eq 0 ] && [ "$rc_ok" -eq 1 ]; then
        printf "${GREEN}PASS${NC}\n"
        PASSED=$((PASSED + 1))
    else
        printf "${RED}FAIL${NC}\n"
        FAILED=$((FAILED + 1))
    fi
}

echo "============================================================"
echo "          Running test suite on $(uname -s) $(uname -r)     "
echo "============================================================"

setup_sandbox

# 16 standard test cases according to report
run_test "1. Default listing" "-1" "$SANDBOX"
run_test "2. Hidden entries (-a)" "-1a" "$SANDBOX"
run_test "3. Almost all (-A)" "-1A" "$SANDBOX"
run_test "4. Long format (-l)" "-l" "$SANDBOX"
run_test "5. Numeric IDs (-n)" "-n" "$SANDBOX"
run_test "6. Override -l vs -n (-ln)" "-ln" "$SANDBOX"
run_test "7. Time sort (-1t)" "-1t" "$SANDBOX"
run_test "8. Size sort (-1S)" "-1S" "$SANDBOX"
run_test "9. Reverse order (-1r)" "-1r" "$SANDBOX"
run_test "10. Inode display (-1i)" "-1i" "$SANDBOX"
run_test "11. Block counts (-1s)" "-1s" "$SANDBOX"
run_test "12. Human readable (-lh)" "-lh" "$SANDBOX"
run_test "13. Classification flag (-1F)" "-1F" "$SANDBOX"
run_test "14. Directory itself (-1d)" "-1d" "$SANDBOX"
run_test "15. Recursive listing (-1R)" "-1R" "$SANDBOX/dir1"
run_test "16. Non-existent error" "-1" "$SANDBOX/non_existent_file"

cleanup_sandbox

echo "============================================================"
if [ "$FAILED" -eq 0 ]; then
    printf "${GREEN}All tests passed (%d/%d)${NC}\n" "$PASSED" "$TOTAL"
    exit 0
else
    printf "${RED}Some tests failed: %d failed, %d passed${NC}\n" "$FAILED" "$PASSED"
    exit 1
fi
