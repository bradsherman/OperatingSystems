#!/bin/bash

pol=$1
echo "Algorithm, # Frames, # Reads, # Writes, # Page Faults" >> ${pol}.csv
for alg in "sort" "scan" "focus"; do
    for i in 10 20 30 40 50 60 70 80 90 100; do
        out=$(./virtmem 100 $i $pol $alg)
        reads=$(echo "$out" | grep "Reads" | cut -d " " -f 3)
        writes=$(echo "$out" | grep "Writes" | cut -d " " -f 3)
        faults=$(echo "$out" | grep "Page Faults" | cut -d " " -f 4)
        echo "$alg,$i,$reads,$writes,$faults" >> ${pol}.csv
    done
done
