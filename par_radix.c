#include <limits.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mt19937-64.c"

void swap(uint64_t **a, uint64_t **b) {
  uint64_t *tmp = *a;
  *a = *b;
  *b = tmp;
}

int is_sorted(uint64_t *arr, int n) {
  for (int i = 1; i < n; i++) {
    if (arr[i - 1] > arr[i]) {
      return 0;
    }
  }
  return 1;
}

void radix_sort_parallel(uint64_t *arr, uint64_t *output, int n, int b, int p) {
  int num_buckets = 1 << b;
  int num_passes = (sizeof(uint64_t) * 8 + b - 1) / b;
  uint64_t mask = num_buckets - 1;

  int *count = (int *)malloc(num_buckets * sizeof(int));
  int *local_count = (int *)malloc(p * num_buckets * sizeof(int));
  double start = omp_get_wtime();

#pragma omp parallel num_threads(p)
  {
    int tid = omp_get_thread_num();
    int *thread_count = &local_count[tid * num_buckets];

    for (int pass = 0; pass < num_passes; pass++) {
      memset(thread_count, 0, num_buckets * sizeof(int));
      int shift = pass * b;

#pragma omp for schedule(static)
      for (int i = 0; i < n; i++) {
        int digit = (arr[i] >> shift) & mask;
        thread_count[digit]++;
      }

#pragma omp single
      {
        memset(count, 0, num_buckets * sizeof(int));
        for (int t = 0; t < p; t++) {
          for (int bucket = 0; bucket < num_buckets; bucket++) {
            count[bucket] += local_count[t * num_buckets + bucket];
          }
        }

        for (int bucket = 1; bucket < num_buckets; bucket++)
          count[bucket] += count[bucket - 1];

        for (int bucket = 0; bucket < num_buckets; bucket++) {
          int base = (bucket == 0) ? 0 : count[bucket - 1];
          for (int t = 0; t < p; t++) {
            int c = local_count[t * num_buckets + bucket];
            local_count[t * num_buckets + bucket] = base;
            base += c;
          }
        }
      }

      int *offsets = &local_count[tid * num_buckets];
#pragma omp for schedule(static)
      for (int i = 0; i < n; i++) {
        int digit = (arr[i] >> shift) & mask;
        int pos = offsets[digit]++;
        output[pos] = arr[i];
      }

#pragma omp single
      swap(&arr, &output);
    }
  }

  double end = omp_get_wtime();

  if (is_sorted(arr, n))
    printf("Time: %lf\n", (end - start));
  else
    printf("Array not sorted\n");

  free(count);
  free(local_count);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: %s <array size> <bits per digit> <num threads>\n", argv[0]);
    return 1;
  }

  char *end;
  int n = strtol(argv[1], &end, 10);
  if (*end != '\0' || n <= 0) {
    printf("Invalid array size\n");
    return 1;
  }

  int b = strtol(argv[2], &end, 10);
  if (*end != '\0' || b <= 0 || b > 32) {
    printf("Invalid bits per digit\n");
    return 1;
  }

  int p = strtol(argv[3], &end, 10);
  int max_threads = omp_get_max_threads();
  if (*end != '\0' || p <= 0 || p > max_threads) {
    printf("Invalid thread count: choose 0 < p < %d\n", max_threads);
    return 1;
  }

  uint64_t *arr = (uint64_t *)malloc(n * sizeof(uint64_t));
  uint64_t *output = (uint64_t *)malloc(n * sizeof(uint64_t));
  init_genrand64((uint64_t)(time(NULL)));
  for (int i = 0; i < n; i++) {
    arr[i] = (uint64_t)genrand64_int64();
  }

  radix_sort_parallel(arr, output, n, b, p);

  free(arr);
  free(output);
  return 0;
}
