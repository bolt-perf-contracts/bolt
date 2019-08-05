#!/bin/bash

echo "[init] Initializing middlebox..."

sudo apt-get -qq update

sudo apt-get install -yqq \
    tcpdump git libpcap-dev \
    linux-headers-3.13.0-93 \
    libglib2.0-dev daemon iperf3 netperf tmux

pushd "$CASE_ROOT"
  bash bolt/install.sh dpdk-only
popd
