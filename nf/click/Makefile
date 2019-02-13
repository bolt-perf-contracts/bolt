ifndef CLICK_FILE
$(Please define CLICK_FILE to the name of the file you'd like to run)
endif

# Binary name
APP := click

# Runtime and verification files
NF_FILES := $(CLICK_FILE)

# Verification files
# NF_VERIF_FILES := 

# Verification arguments
# NF_VERIF_ARGS := 

# Include parent (in a convoluted way cause of DPDK)
include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
