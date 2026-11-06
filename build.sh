#!/usr/bin/env bash
set -Eeuo pipefail
shopt -s nullglob

# author: Danila "akshin_" Axyonov
# -------------------------

usage() {
    cat <<EOF
Usage:
    $0 [OPTIONS] [ROOT]

Compile all C source files (*.c) in ROOT.

Arguments:
    ROOT          root directory containing C source files
                  (default: current directory)

Options:
    -h, --help    show this help message and exit

The script can read the following parameters from C source files:
    // Compiler: compiler_name
    // Output: output_filename

If the parameters are not specified, the defaults are:
    Compiler:     clang
    Output:       main.out
EOF
}

ROOT="."
root_set=0

while (( $# > 0 )); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;

        -*)
            printf 'Error: unknown option: %s\n' "$1" >&2
            printf 'Try "%s --help" for more information.\n' "$0" >&2
            exit 1
            ;;

        *)
            if (( root_set )); then
                printf 'Error: multiple ROOT directories specified.\n' >&2
                printf 'Try "%s --help" for more information.\n' "$0" >&2
                exit 1
            fi
            root_set=1

            ROOT="$1"
            if [[ ! -d "$ROOT" ]]; then
                printf 'Error: ROOT is not a directory: %s\n' "$ROOT" >&2
                exit 1
            fi
            ;;
    esac

    shift
done

ROOT="$(realpath "$ROOT")"

# -------------------------

find_parameter() {
    local parameter_attempt parameter=""
    for file in "$ROOT"/*.c; do
        parameter_attempt="$(sed -n "s#^// ${1}: ##Ip" "$file")"

        if [[ -n "$parameter_attempt" ]]; then
            if [[ -n "$parameter" ]]; then
                printf 'Extra value \"%s\" for \"%s\" parameter has been found in %s.\n' "$parameter_attempt" "$1" "$file" >&2
                return 1
            fi

            parameter="$parameter_attempt"
            printf '\"%s\" parameter has been set to \"%s\" (found in %s).\n' "$1" "$parameter" "$file"
        fi
    done

    echo "$parameter" >&3
    return 0
}

# -------------------------

tmpdir="$(mktemp -d)"
trap 'rm -rf -- "$tmpdir"' EXIT INT TERM HUP

logfile="compile.log"
touch "$tmpdir/$logfile"

error_handler() {
    local status="$?"
    printf '[ERROR] exit status: %d\n' "$status" >&2
    printf '[DETAILS] ' >&2
    cat "$tmpdir/$logfile" >&2
}
trap error_handler ERR

# -------------------------

# to make extra auto-generated files appear in the tmpdir
# so they will be deleted with it
cd "$tmpdir"

compiler="clang"
compiler_attempt_file="compiler_attempt"
touch "$tmpdir/$compiler_attempt_file"
find_parameter "compiler" 2>> "$tmpdir/$logfile" 3> "$tmpdir/$compiler_attempt_file"
compiler_attempt="$(cat "$tmpdir/$compiler_attempt_file")"
if [[ -n "$compiler_attempt" ]]; then
    compiler="$compiler_attempt"
fi

output="main.out"
output_attempt_file="output_attempt"
touch "$tmpdir/$output_attempt_file"
find_parameter "output" 2>> "$tmpdir/$logfile" 3> "$tmpdir/$output_attempt_file"
output_attempt="$(cat "$tmpdir/$output_attempt_file")"
if [[ -n "$output_attempt" ]]; then
    output="$output_attempt"
fi

flags=""
flags_attempt_file="flags_attempt"
touch "$tmpdir/$flags_attempt_file"
find_parameter "flags" 2>> "$tmpdir/$logfile" 3> "$tmpdir/$flags_attempt_file"
flags_attempt="$(cat "$tmpdir/$flags_attempt_file")"
if [[ -n "$flags_attempt" ]]; then
    flags="$flags_attempt"
fi

for file in "$ROOT"/*.c; do
    "$compiler" -c "$file" -o "$tmpdir/$(basename "$file" .c).o" $flags 2>> "$tmpdir/$logfile"
done

"$compiler" "$tmpdir"/*.o -o "$ROOT/$output" $flags 2>> "$tmpdir/$logfile"

echo "Done."
