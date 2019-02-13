#pragma once
#include <stdint.h>

#include <stdint.h>


static inline uint16_t
rte_cpu_to_be_16(uint16_t x)
{
	return ((x & 0xFF) << 8) | (x >> 8);
}

static inline uint32_t
rte_be_to_cpu_32(uint32_t x)
{
	return __builtin_bswap32(x);
}
