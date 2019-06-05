#!/bin/bash

. paths.sh

set -euxo pipefail


pushd nf/vignat
  make verify-dpdk
popd
pushd nf/bridge
  make verify-dpdk
popd
pushd nf/vigbalancer
  make verify-dpdk
popd 
pushd nf/lpm
  make verify-dpdk
popd 

echo "All symbex succeeded"

