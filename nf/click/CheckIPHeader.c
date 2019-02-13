#include "click/nf_click.h"

// NOTE: Symnet oddly defines MinPacketSize as 64, even though an IPv4 packet can be 20 bytes long...

int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	// inputPortName(0)
	DROP_UNLESS(mbuf->port == INPUT_PORT);

	// Constrain(IPVersionOffset,:==:(ConstantValue(4)))
	struct ipv4_hdr* ipv4_header = nf_get_mbuf_ipv4_header(mbuf);
        DROP_UNLESS(ipv4_header != NULL);

	// Constrain(IPLengthOffset,:>=:(ConstantValue(MinPacketSize)))
	DROP_UNLESS(ipv4_header->total_length >= 64);

	// Constrain(IPHeaderLengthOffset,:>=:(ConstantValue(MinPacketSize)))
	uint16_t ihl = (ipv4_header->version_ihl & IPV4_HDR_IHL_MASK) * IPV4_IHL_MULTIPLIER;
	DROP_UNLESS(ihl >= 20);

	// Constrain(IPLengthOffset,:>=:(:@(IPHeaderLengthOffset)))
	DROP_UNLESS(ipv4_header->total_length >= ihl);

	// Forward(outputPortName(0))
	return OUTPUT_PORT;
}
