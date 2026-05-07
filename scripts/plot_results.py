#!/usr/bin/env python3
"""
Plot serial-vs-MPI Collatz benchmark results.

Usage:
    python3 scripts/plot_results.py results/benchmark.csv results/performance.png
"""

import sys
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: plot_results.py input.csv output.png", file=sys.stderr)
        return 1

    csv_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])

    df = pd.read_csv(csv_path)
    if df.empty:
        raise ValueError("Benchmark CSV is empty.")

    summary = (
        df.groupby(["algorithm", "ranks"], as_index=False)
        .agg(mean_seconds=("seconds", "mean"), std_seconds=("seconds", "std"))
        .sort_values(["algorithm", "ranks"])
    )

    serial = summary[summary["algorithm"] == "serial"]
    mpi = summary[summary["algorithm"] == "mpi"]

    plt.figure(figsize=(8, 5))

    if not serial.empty:
        serial_time = float(serial.iloc[0]["mean_seconds"])
        max_rank = int(max(df["ranks"]))
        plt.axhline(serial_time, linestyle="--", label="serial baseline")

    if not mpi.empty:
        plt.errorbar(
            mpi["ranks"],
            mpi["mean_seconds"],
            yerr=mpi["std_seconds"].fillna(0.0),
            marker="o",
            capsize=4,
            label="MPI",
        )

    plt.xlabel("MPI ranks")
    plt.ylabel("Runtime, seconds")
    plt.title("Collatz finite-range verification performance")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=200)

    table_path = out_path.with_suffix(".summary.csv")
    summary.to_csv(table_path, index=False)

    print(f"Wrote {out_path}")
    print(f"Wrote {table_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
