#!/usr/bin/env bash
set -euo pipefail

L="${L:-1}"
U="${U:-10000000}"
MAX_STEPS="${MAX_STEPS:-10000}"
REPEATS="${REPEATS:-3}"
RANKS="${RANKS:-1 2 4 8}"
OUT="${OUT:-results/benchmark.csv}"

mkdir -p results build
make

echo "algorithm,ranks,repeat,L,U,max_steps,checked,seconds,status" > "$OUT"

parse_field() {
    local key="$1"
    local file="$2"
    awk -F',' -v k="$key" '$1 == k { print $2 }' "$file"
}

for rep in $(seq 1 "$REPEATS"); do
    tmp="results/serial_rep_${rep}.out"
    ./build/collatz_serial "$L" "$U" "$MAX_STEPS" > "$tmp"

    status=$(parse_field status "$tmp")
    checked=$(parse_field checked "$tmp")
    seconds=$(parse_field seconds "$tmp")

    echo "serial,1,$rep,$L,$U,$MAX_STEPS,$checked,$seconds,$status" >> "$OUT"
done

for ranks in $RANKS; do
    for rep in $(seq 1 "$REPEATS"); do
        tmp="results/mpi_${ranks}_rep_${rep}.out"
        mpirun -np "$ranks" ./build/collatz_mpi "$L" "$U" "$MAX_STEPS" > "$tmp"

        status=$(parse_field status "$tmp")
        checked=$(parse_field checked "$tmp")
        seconds=$(parse_field seconds "$tmp")

        echo "mpi,$ranks,$rep,$L,$U,$MAX_STEPS,$checked,$seconds,$status" >> "$OUT"
    done
done

printf 'Wrote %s\n' "$OUT"
