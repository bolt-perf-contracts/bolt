#!/bin/sh

# DecIPTTL Unconstrained
make CLICK_FILE=DecIPTTL.c MEASURE_OUTPUT_FILE=perf_decipttl_unconstrained.log PCAP_FILE=pcap/unirand10000.pcap measure

# Discard Unconstrained
make CLICK_FILE=Discard.c MEASURE_OUTPUT_FILE=perf_discard_unconstrained.log PCAP_FILE=pcap/unirand10000.pcap measure
