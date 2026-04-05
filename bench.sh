#!/bin/bash
URL=$1
RUNS=${2:-7}
RESULTS=()

echo "Running $RUNS iterations against $URL"
for i in $(seq 1 $RUNS); do
    # let system settle between runs
    sleep 2
    RPS=$(wrk -t8 -c400 -d15s "$URL" 2>/dev/null \
        | grep "Requests/sec" | awk '{print $2}')
    RESULTS+=($RPS)
    echo "  run $i: $RPS req/s"
done

# sort and take median
SORTED=($(printf '%s\n' "${RESULTS[@]}" | sort -n))
MID=$(( ${#SORTED[@]} / 2 ))
echo "Median: ${SORTED[$MID]} req/s"
echo "Min: ${SORTED[0]} req/s"
echo "Max: ${SORTED[-1]} req/s"
