#!/bin/bash
KLEE_DIR=~/projects/Bolt/klee

set -e

NF=$1
TEST=${2:-all}

if [ $NF != "vignat" ] && [ $NF != "bridge" ] && [ $NF != "vigbalancer" ]; then
  echo "Unsupported parameter"
	echo $TEST
  exit
fi


if [ "$TEST" != "all" ] && [ "$TEST" != "contracts-only" ]; then
  echo "Unsupported parameter"
	echo $TEST
  exit
fi

if [ "$TEST" == "all" ]; then 
	make verify-dpdk
	make executable-$NF
	make -j $(nproc) instr-traces
fi

pushd klee-last
	$KLEE_DIR/scripts/process-traces.sh . verify-dpdk Num_bucket_traversals 1 Num_hash_collisions 0  expired_flows 0 \
		  timestamp_option 0 lpm_stages 1
popd

