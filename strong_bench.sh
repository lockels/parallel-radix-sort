#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <N> <B>"
  exit 1
fi

N=$1
B=$2
EXEC=./par_radix

MAX_THREADS=${OMP_NUM_THREADS:-10}
RUNS=3

for p in $(seq 1 $MAX_THREADS); do
  echo "=============================="
  echo "Running with p=$p threads"
  echo "Running best of 3..."
  echo "=============================="

  best_time=""
  for r in $(seq 1 $RUNS); do
    output=$($EXEC $N $B $p)
    time=$(echo "$output" | grep "Time:" | awk '{print $2}')
    if [ -n "$time" ]; then
      if [ -z "$best_time" ] || (($(echo "$time < $best_time" | bc -l))); then
        best_time=$time
      fi
    fi
  done

  if [ -n "$best_time" ]; then
    echo "Time: $best_time"
  else
    echo "Array not sorted"
  fi
done
