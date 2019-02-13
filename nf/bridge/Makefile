# Binary name
APP := bridge

# Runtime and verification files
NF_FILES := bridge_forward.c bridge_config.c bridge_data.c

# Verification files
NF_VERIF_FILES := bridge_loop.c

# Verification arguments
NF_VERIF_ARGS := --expire 10 --capacity 32768 --config no-file.cfg

# Include parent (in a convoluted way cause of DPDK)
include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
