/*
 * collatz_serial.c
 *
 * Standard single-process Collatz finite-range verifier.
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
typedef unsigned __int128 u128;
#else
#error "This program requires unsigned __int128 support. Use GCC or Clang."
#endif

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
}

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

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s L U max_steps\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint64_t L = strtoull(argv[1], NULL, 10);
    uint64_t U = strtoull(argv[2], NULL, 10);
    uint64_t max_steps = strtoull(argv[3], NULL, 10);

    if (L == 0 || U < L) {
        fprintf(stderr, "Invalid interval. Require 1 <= L <= U.\n");
        return EXIT_FAILURE;
    }

    double t0 = now_seconds();
    uint64_t checked = 0;

    for (uint64_t s = L; s <= U; s++) {
        u128 final_value = 0;

        if (!verify_seed(s, max_steps, &final_value)) {
            double t1 = now_seconds();
            printf("status,unverified\n");
            printf("seed,%" PRIu64 "\n", s);
            printf("final_value,");
            print_u128(final_value);
            printf("\n");
            printf("checked,%" PRIu64 "\n", checked);
            printf("seconds,%.9f\n", t1 - t0);
            return EXIT_FAILURE;
        }

        checked++;

        if (s == UINT64_MAX) {
            break;
        }
    }

    double t1 = now_seconds();

    printf("status,verified\n");
    printf("algorithm,serial\n");
    printf("ranks,1\n");
    printf("L,%" PRIu64 "\n", L);
    printf("U,%" PRIu64 "\n", U);
    printf("max_steps,%" PRIu64 "\n", max_steps);
    printf("checked,%" PRIu64 "\n", checked);
    printf("seconds,%.9f\n", t1 - t0);

    return EXIT_SUCCESS;
}
