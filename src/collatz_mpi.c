/*
 * collatz_mpi.c
 *
 * MPI finite-range Collatz verifier.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#if defined(__GNUC__) || defined(__clang__)
typedef unsigned __int128 u128;
#else
#error "This program requires unsigned __int128 support. Use GCC or Clang."
#endif

static void print_u128(u128 x)
{
    if (x == 0) {
        putchar('0');
        return;
    }

    char buf[64];
    int i = 0;

    while (x > 0) {
        buf[i++] = (char)('0' + (int)(x % 10));
        x /= 10;
    }

    while (i > 0) {
        putchar(buf[--i]);
    }
}

static int verify_seed(uint64_t seed, uint64_t max_steps, u128 *final_value)
{
    u128 n = seed;

    for (uint64_t step = 0; step < max_steps; step++) {
        if (n == 1) {
            *final_value = n;
            return 1;
        }

        if (n < seed) {
            *final_value = n;
            return 1;
        }

        if ((n & 1u) == 0u) {
            n >>= 1;
        } else {
            if (n > (((u128)-1) - 1u) / 3u) {
                *final_value = n;
                return 0;
            }
            n = 3u * n + 1u;
        }
    }

    *final_value = n;
    return 0;
}

static void compute_block(uint64_t L,
                          uint64_t U,
                          int rank,
                          int size,
                          uint64_t *local_start,
                          uint64_t *local_end)
{
    uint64_t N = U - L + 1u;
    uint64_t base = N / (uint64_t)size;
    uint64_t rem = N % (uint64_t)size;
    uint64_t count;
    uint64_t offset;

    if ((uint64_t)rank < rem) {
        count = base + 1u;
        offset = (uint64_t)rank * count;
    } else {
        count = base;
        offset = rem * (base + 1u) + ((uint64_t)rank - rem) * base;
    }

    *local_start = L + offset;
    *local_end = count == 0 ? *local_start - 1u : *local_start + count - 1u;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 4) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s L U max_steps\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    uint64_t L = strtoull(argv[1], NULL, 10);
    uint64_t U = strtoull(argv[2], NULL, 10);
    uint64_t max_steps = strtoull(argv[3], NULL, 10);

    if (L == 0 || U < L) {
        if (rank == 0) {
            fprintf(stderr, "Invalid interval. Require 1 <= L <= U.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    uint64_t local_start = 0;
    uint64_t local_end = 0;
    compute_block(L, U, rank, size, &local_start, &local_end);

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    int local_ok = 1;
    uint64_t local_bad_seed = UINT64_MAX;
    uint64_t local_checked = 0;
    u128 bad_final = 0;

    if (local_start <= local_end) {
        for (uint64_t s = local_start; s <= local_end; s++) {
            u128 final_value = 0;

            if (!verify_seed(s, max_steps, &final_value)) {
                local_ok = 0;
                local_bad_seed = s;
                bad_final = final_value;
                break;
            }

            local_checked++;

            if (s == UINT64_MAX) {
                break;
            }
        }
    }

    int global_ok = 0;
    MPI_Allreduce(&local_ok, &global_ok, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

    uint64_t global_checked = 0;
    MPI_Reduce(&local_checked, &global_checked, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);

    uint64_t global_bad_seed = UINT64_MAX;
    MPI_Reduce(&local_bad_seed, &global_bad_seed, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD);

    double t1 = MPI_Wtime();
    double local_seconds = t1 - t0;
    double max_seconds = 0.0;
    MPI_Reduce(&local_seconds, &max_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        if (global_ok) {
            printf("status,verified\n");
        } else {
            printf("status,unverified\n");
            printf("seed,%" PRIu64 "\n", global_bad_seed);
        }

        printf("algorithm,mpi\n");
        printf("ranks,%d\n", size);
        printf("L,%" PRIu64 "\n", L);
        printf("U,%" PRIu64 "\n", U);
        printf("max_steps,%" PRIu64 "\n", max_steps);
        printf("checked,%" PRIu64 "\n", global_checked);
        printf("seconds,%.9f\n", max_seconds);
    }

    if (!local_ok) {
        fprintf(stderr, "rank,%d,bad_seed,%" PRIu64 ",final_value,", rank, local_bad_seed);
        print_u128(bad_final);
        fprintf(stderr, "\n");
    }

    MPI_Finalize();
    return global_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
