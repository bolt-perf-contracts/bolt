#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <rte_mbuf.h>

#include "lib/stubs/mbuf_content.h"


// ifdef this so the stdio stubs don't create files that we don't need
#ifdef VIGOR_STUB_HARDWARE
#  ifdef STUB_DEVICES_COUNT
#    define STUB_HARDWARE_DEVICES_COUNT STUB_DEVICES_COUNT
#  else
#    define STUB_HARDWARE_DEVICES_COUNT 2
#  endif
#else
#  define STUB_HARDWARE_DEVICES_COUNT 0
#endif



struct stub_device {
        int id;
	char* name;

	int interrupts_fd;
	bool interrupts_enabled;

	volatile void* mem; // intercepted by stub
	size_t mem_len;
	volatile void* mem_shadow; // used as the backing store

	int16_t current_mdi_address; // -1 == none

	int32_t i2c_state; // see i2cctl implementation
	uint8_t i2c_counter; // number of bits, N/ACKs included, since the start of the current operation
	uint8_t i2c_address; // address of the current operation
	uint64_t i2c_start_time; // time of last START
	uint64_t i2c_clock_time; // time of last clock change
	uint64_t i2c_stop_time; // time of last stop

	uint8_t sfp_address; // see i2cctl sfp implementation

	// required for the reset hack...
	uint64_t old_mbuf_addr;
};

struct stub_device DEVICES[STUB_HARDWARE_DEVICES_COUNT];

// Required because the validator expects the traced mbuf and its content to stay at the same address throughout.
// Sound as long as RX and TX are each called once at most, which we check.
struct rte_mbuf traced_mbuf;
struct stub_mbuf_content traced_mbuf_content;



#ifdef VIGOR_STUB_HARDWARE
void stub_hardware_receive_packet(uint16_t device);
// HACK this should not be needed :( but it is cause of the current impl. of havocing
void stub_hardware_reset_receive(uint16_t device);
#else
static inline void stub_hardware_receive_packet(uint16_t device) { }
static inline void stub_hardware_reset_receive(uint16_t device) { }
#endif
