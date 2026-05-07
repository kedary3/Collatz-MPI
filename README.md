# Parallel Collatz Verification in C and MPI

This repository contains two finite-range Collatz verification programs:

1. `collatz_serial.c`: a standard single-process C implementation.
2. `collatz_mpi.c`: a parallel MPI implementation that distributes seed intervals across ranks.

The project also includes benchmark and plotting scripts to compare serial and MPI performance.

This code does **not** prove the Collatz conjecture. It verifies that every seed in a finite interval `[L, U]` reaches `1` or falls below its starting value within a specified step cutoff.

## Repository layout

```text
.
├── Makefile
├── README.md
├── src
│   ├── collatz_serial.c
│   └── collatz_mpi.c
├── scripts
│   ├── benchmark.sh
│   └── plot_results.py
└── results
    └── .gitkeep
```

## Requirements

For the serial program:

- C compiler supporting C11, such as `gcc` or `clang`

For the MPI program:

- MPI implementation, such as OpenMPI or MPICH
- `mpicc`
- `mpirun` or `mpiexec`

For visualization:

- Python 3
- `matplotlib`
- `pandas`

Install Python dependencies:

```bash
python3 -m pip install matplotlib pandas
```

## Build

```bash
make
```

This creates:

```text
build/collatz_serial
build/collatz_mpi
```

To remove build artifacts:

```bash
make clean
```

## Run the serial verifier

```bash
./build/collatz_serial 1 10000000 10000
```

Arguments:

```text
L          lower seed, inclusive
U          upper seed, inclusive
max_steps  maximum Collatz iterations per seed
```

## Run the MPI verifier

```bash
mpirun -np 4 ./build/collatz_mpi 1 10000000 10000
```

## Benchmark

Run the included benchmark script:

```bash
bash scripts/benchmark.sh
```

By default, it tests:

- serial execution
- MPI with 1, 2, 4, and 8 ranks

The output is written to:

```text
results/benchmark.csv
```

You can override the benchmark parameters:

```bash
L=1 U=50000000 MAX_STEPS=10000 REPEATS=3 RANKS="1 2 4 8 16" bash scripts/benchmark.sh
```

## Plot performance

```bash
python3 scripts/plot_results.py results/benchmark.csv results/performance.png
```

The plot compares mean runtime for the serial implementation and the MPI implementation at different rank counts.

## Method

The serial verifier checks each seed one after another.

The MPI verifier partitions `[L, U]` into disjoint blocks:

```text
[L, U] = I_0 ∪ I_1 ∪ ... ∪ I_{P-1}
```

Each MPI rank verifies one block independently. The ranks then use a global reduction to determine whether every block was verified.

Conceptually, the MPI computation has the structure:

```text
Partition
⊗
(Worker_0 ⊕ Worker_1 ⊕ ... ⊕ Worker_{P-1})
⊗
GlobalReduction
```

Here, the worker layer is parallel because each seed trajectory can be evaluated independently.

## Notes

The implementation uses `unsigned __int128` internally to reduce overflow risk. This is supported by GCC and Clang on most 64-bit platforms. Very large seed ranges may still exceed 128-bit arithmetic during intermediate `3n + 1` steps.

