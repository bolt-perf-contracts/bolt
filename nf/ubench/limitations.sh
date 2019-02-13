#!/bin/bash
CASTAN_DIR=/home/rishabh/castan

rm artificial.log
rm clear_cache.log
rm real.log 

make cache-limitation-ubench

for i in `seq 1 1000`;
do
      ./map_ubench_artificial >> artificial.log                
done

for i in `seq 1 10000`;
do
      ./map_ubench_clear_cache >> clear_cache.log
done

for i in `seq 1 100000`;
do
      ./map_ubench_real >> real.log
done

awk '/Time is/{print $3}' artificial.log > artificial.csv
awk '/Time is/{print $3}' clear_cache.log > clear_cache.csv
awk '/Time is/{print $3}' real.log > real.csv

$CASTAN_DIR/scripts/plot-cdf.sh cache-limitation.png Latency 0:4000 artificial.csv artifical clear_cache.csv clear-cache real.csv real predicted.csv Bolt-prediction

rm *.cdf
