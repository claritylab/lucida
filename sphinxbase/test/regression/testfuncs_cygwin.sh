# Utility functions and parameters for regression tests

# Predefined directories you may need
: ${CONFIGURATION:=Debug}
builddir="../../bin/$CONFIGURATION"
sourcedir="../.."
tests=$sourcedir/test

# Automatically report failures on exit
failures=""
trap "report_failures" 0

run_program() {
    program=`basename $1`
    shift
    "$builddir/$program" $@
}

debug_program() {
    program=`basename $1`
    shift
    gdb --args "$builddir/$program" $@
}

memcheck_program() {
    program=`basename $1`
    shift
    valgrind --leak-check=full "$builddir/$program" $@
}

pass() {
    title="$1"
    echo "$title PASSED"
}

fail() {
    title="$1"
    echo "$title FAILED"
    failures="$failures,$title"
}

compare_table() {
    title="$1"
    shift
    if perl "$tests/compare_table.pl" $@ | grep SUCCESS >/dev/null 2>&1; then
	pass "$title"
    else
	fail "$title"
    fi 
}

report_failures() {
    if test x"$failures" = x; then
	echo "All sub-tests passed"
	exit 0
    else
	echo "Sub-tests failed:$failures" | sed -e 's/,/ /g'
	exit 1
    fi
}
