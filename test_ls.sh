#!/usr/bin/env bash
# ==============================================================================
# test_ls.sh - Automated Test Suite for NetBSD ls(1) Midterm Project
# Tests all 19 options: [-AacdFfhiklnqRrSstuw], overrides, edge cases & Valgrind
# ==============================================================================

set -u

MYLS="./myls"
SYSLS="/bin/ls"
PASSED=0
FAILED=0
TOTAL=0
SANDBOX="test_sandbox"

# ANSI Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Build project if binary doesn't exist
if [ ! -x "$MYLS" ]; then
    echo -e "${YELLOW}Binary $MYLS not found. Compiling with make...${NC}"
    make clean && make
    if [ ! -x "$MYLS" ]; then
        echo -e "${RED}Error: Build failed.${NC}"
        exit 1
    fi
fi

setup_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX/dir1/subdir1"
    mkdir -p "$SANDBOX/dir2"
    mkdir -p "$SANDBOX/empty_dir"
    mkdir -p "$SANDBOX/.hidden_dir"

    # Files with different sizes and timestamps
    echo "Small" > "$SANDBOX/file_a.txt"
    touch -t 202101010000 "$SANDBOX/file_a.txt"

    echo "Medium content inside file b for testing size sort." > "$SANDBOX/file_b.txt"
    touch -t 202306011200 "$SANDBOX/file_b.txt"

    echo "Largest file payload with plenty of text data to ensure size sorting works properly across different block sizes." > "$SANDBOX/file_c.txt"
    touch -t 202409010830 "$SANDBOX/file_c.txt"

    echo "Hidden content" > "$SANDBOX/.hidden_file"
    echo "Sub content" > "$SANDBOX/dir1/subdir1/nested.txt"

    # Executable file
    echo '#!/bin/sh' > "$SANDBOX/script.sh"
    chmod +x "$SANDBOX/script.sh"

    # Symbolic links
    ln -s "file_a.txt" "$SANDBOX/symlink_ok"
    ln -s "non_existent_target" "$SANDBOX/dangling_symlink"
    ln -s "dir1" "$SANDBOX/symlink_dir"
    ln -s ".." "$SANDBOX/dir1/subdir1/loop_link" 2>/dev/null || true

    # FIFO and Socket if system allows
    mkfifo "$SANDBOX/test_fifo" 2>/dev/null || true
}

cleanup_sandbox() {
    rm -rf "$SANDBOX"
    rm -f /tmp/sysls.out /tmp/sysls.err /tmp/myls.out /tmp/myls.err /tmp/valgrind.log
}

run_test() {
    local category="$1"
    local flags="$2"
    shift 2
    local args=("$@")

    TOTAL=$((TOTAL + 1))
    printf "${CYAN}[TEST %02d]${NC} %-28s | Cmd: ${YELLOW}%s %s %s${NC}\n" \
           "$TOTAL" "$category" "$MYLS" "$flags" "${args[*]}"

    # Run system ls (with LC_ALL=C for predictable time/sort)
    LC_ALL=C $SYSLS $flags "${args[@]}" > /tmp/sysls.out 2> /tmp/sysls.err
    local sys_rc=$?

    # Run our myls
    LC_ALL=C $MYLS $flags "${args[@]}" > /tmp/myls.out 2> /tmp/myls.err
    local my_rc=$?

    local diff_out
    diff_out=$(diff -u /tmp/sysls.out /tmp/myls.out 2>&1)
    local diff_rc=$?

    # Normalize exit code check: 0 matches 0; >0 matches >0
    local rc_ok=0
    if [ "$sys_rc" -eq 0 ] && [ "$my_rc" -eq 0 ]; then
        rc_ok=1
    elif [ "$sys_rc" -ne 0 ] && [ "$my_rc" -ne 0 ]; then
        rc_ok=1
    fi

    if [ $diff_rc -eq 0 ] && [ $rc_ok -eq 1 ]; then
        echo -e "         ${GREEN}✓ PASSED${NC} (Exit Code: $my_rc)"
        PASSED=$((PASSED + 1))
    else
        echo -e "         ${RED}✗ FAILED${NC}"
        if [ $diff_rc -ne 0 ]; then
            echo "         --- Output Diff (sysls < vs myls >) ---"
            diff -u /tmp/sysls.out /tmp/myls.out | head -n 15 | sed 's/^/         /'
        fi
        if [ $rc_ok -ne 1 ]; then
            echo -e "         --- Exit code mismatch: sysls ($sys_rc) vs myls ($my_rc) ---"
        fi
        FAILED=$((FAILED + 1))
    fi
}

echo "=========================================================================="
echo "          NETBSD LS(1) COMPLIANCE & VERIFICATION TEST SUITE               "
echo "=========================================================================="

setup_sandbox

# 1. Basic & Hidden Entries (-a, -A)
run_test "Default listing" "-1" "$SANDBOX"
run_test "All entries (-a)" "-1a" "$SANDBOX"
run_test "Almost all entries (-A)" "-1A" "$SANDBOX"

# 2. Long format & Numeric IDs (-l, -n)
run_test "Long format (-l)" "-l" "$SANDBOX"
run_test "Long format all (-la)" "-la" "$SANDBOX"
run_test "Numeric IDs (-n)" "-n" "$SANDBOX"
run_test "Override: -l then -n (-ln)" "-ln" "$SANDBOX"
run_test "Override: -n then -l (-nl)" "-nl" "$SANDBOX"

# 3. Time sorting & Timestamps (-t, -c, -u)
run_test "Sort by mtime (-1t)" "-1t" "$SANDBOX"
run_test "Sort by ctime (-1tc)" "-1tc" "$SANDBOX"
run_test "Sort by atime (-1tu)" "-1tu" "$SANDBOX"
run_test "Override: -c then -u (-1tcu)" "-1tcu" "$SANDBOX"
run_test "Override: -u then -c (-1tuc)" "-1tuc" "$SANDBOX"

# 4. Size sorting & Dimensions (-S, -s, -h, -k)
run_test "Sort by file size (-1S)" "-1S" "$SANDBOX"
run_test "Sort by size reversed (-1Sr)" "-1Sr" "$SANDBOX"
run_test "Block counts (-1s)" "-1s" "$SANDBOX"
run_test "Block counts in KB (-1sk)" "-1sk" "$SANDBOX"
run_test "Human readable sizes (-lh)" "-lh" "$SANDBOX"
run_test "Override: -h then -k (-1shk)" "-1shk" "$SANDBOX"
run_test "Override: -k then -h (-1skh)" "-1skh" "$SANDBOX"

# 5. Reverse & No-sort (-r, -f)
run_test "Reverse alphabetical (-1r)" "-1r" "$SANDBOX"
run_test "No sort / raw dir order (-1f)" "-1f" "$SANDBOX"

# 6. Inode & Classification (-i, -F)
run_test "Inode numbers (-1i)" "-1i" "$SANDBOX"
run_test "Classification types (-1F)" "-1F" "$SANDBOX"
run_test "Combined -lsihF" "-lsihF" "$SANDBOX"

# 7. Directory itself vs Recursive (-d, -R)
run_test "Directory itself (-1d)" "-1d" "$SANDBOX" "$SANDBOX/dir1"
run_test "Directory itself long (-ld)" "-ld" "$SANDBOX"
run_test "Recursive listing (-1R)" "-1R" "$SANDBOX/dir1"
run_test "Override: -R then -d (-1Rd)" "-1Rd" "$SANDBOX"
run_test "Override: -d then -R (-1dR)" "-1dR" "$SANDBOX/dir1"

# 8. Non-printable chars (-q, -w)
run_test "Force '?' for non-print (-1q)" "-1q" "$SANDBOX"
run_test "Force raw non-print (-1w)" "-1w" "$SANDBOX"
run_test "Override: -w then -q (-1wq)" "-1wq" "$SANDBOX"

# 9. Multiple Operands (Files first, Dirs second)
run_test "Single file operand" "-1" "$SANDBOX/file_a.txt"
run_test "Multiple file operands" "-1" "$SANDBOX/file_a.txt" "$SANDBOX/file_b.txt"
run_test "Mixed file and directory" "-1" "$SANDBOX/file_a.txt" "$SANDBOX/dir1"
run_test "Multiple directories" "-1" "$SANDBOX/dir1" "$SANDBOX/dir2"

# 10. Edge Cases & Error Handling
run_test "Empty directory (-1)" "-1" "$SANDBOX/empty_dir"
run_test "Empty directory long (-l)" "-l" "$SANDBOX/empty_dir"
run_test "Dangling symlink (-l)" "-l" "$SANDBOX/dangling_symlink"
run_test "Non-existent target" "-1" "$SANDBOX/non_existent_file"
run_test "Invalid flag handling" "-z" "$SANDBOX"

# 11. Memory Leak Check with Valgrind
echo "=========================================================================="
echo "                   VALGRIND MEMORY INTEGRITY CHECK                        "
echo "=========================================================================="
if command -v valgrind &> /dev/null; then
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --error-exitcode=99 \
             $MYLS -laRsih -F "$SANDBOX" > /dev/null 2> /tmp/valgrind.log
    val_rc=$?
    if [ $val_rc -eq 99 ]; then
        echo -e "${RED}✗ Valgrind detected memory leaks or illegal memory access!${NC}"
        cat /tmp/valgrind.log
        FAILED=$((FAILED + 1))
    else
        echo -e "${GREEN}✓ Valgrind PASSED: 0 memory leaks, 0 errors.${NC}"
        PASSED=$((PASSED + 1))
    fi
    TOTAL=$((TOTAL + 1))
else
    echo -e "${YELLOW}Notice: valgrind is not installed. Skipping memory leak check.${NC}"
fi

cleanup_sandbox

echo "=========================================================================="
printf "TOTAL TESTS: %d | ${GREEN}PASSED: %d${NC} | ${RED}FAILED: %d${NC}\n" \
       "$TOTAL" "$PASSED" "$FAILED"
echo "=========================================================================="

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: All tests passed with 100%% compliance!${NC}"
    exit 0
else
    echo -e "${RED}FAILURE: Some test cases failed. Check output above.${NC}"
    exit 1
fi
