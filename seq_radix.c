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

void radix_sort_seq(uint64_t *arr, uint64_t *output, int n, int b) {
  int num_buckets = 1 << b;
  int num_passes = (sizeof(uint64_t) * 8 + b - 1) / b;
  uint64_t mask = num_buckets - 1;

  int *count = (int *)malloc(num_buckets * sizeof(int));
  uint64_t **bucket = (uint64_t **)malloc(num_buckets * sizeof(uint64_t *));

  double start = omp_get_wtime();

  for (int pass = 0; pass < num_passes; pass++) {
    int shift = pass * b;

    memset(count, 0, num_buckets * sizeof(int));
    for (int i = 0; i < n; i++) {
      int digit = (arr[i] >> pass * b) & mask;
      count[digit]++;
    }

    uint64_t index = 0;
    for (int b = 0; b < num_buckets; b++) {
      bucket[b] = output + index;
      index += count[b];
    }

    for (int i = 0; i < n; i++) {
      int digit = (arr[i] >> pass * b) & mask;
      *bucket[digit]++ = arr[i];
    }

    swap(&arr, &output);
  }

  double end = omp_get_wtime();

  if (is_sorted(arr, n))
    printf("Time: %lf\n", (end - start));
  else
    printf("Array not sorted\n");

  free(count);
  free(bucket);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <array size> <bits per digit>\n", argv[0]);
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

  uint64_t *arr = (uint64_t *)malloc(n * sizeof(uint64_t));
  uint64_t *output = (uint64_t *)malloc(n * sizeof(uint64_t));
  init_genrand64((uint64_t)(time(NULL)));
  for (int i = 0; i < n; i++) {
    arr[i] = (uint64_t)genrand64_int64();
  }

  radix_sort_seq(arr, output, n, b);

  free(arr);
  free(output);
  return 0;
}
