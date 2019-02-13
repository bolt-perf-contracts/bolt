#include "click/nf_click.h"


int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	int config_value;
	klee_make_symbolic(&config_value, sizeof(int), "config_value");

	// inputPortName(0)
	DROP_UNLESS(mbuf->port == INPUT_PORT);

	// NOTE: Symnet doesn't seem to ensure the packet is IP before checking TTL...
	struct ipv4_hdr* ipv4_header = nf_get_mbuf_ipv4_header(mbuf);
        DROP_UNLESS(ipv4_header != NULL);

	// Assign(TTL,:-:(:@(TTL),ConstantValue(1))),
	// Constrain(TTL, :>:(ConstantValue(0)))
	// NOTE: The Symnet paper specifically mentions that the above behavior causes a bug with unsigned integer - the TTL rolls over and is always >0 even if it was 0 before
	//       so let's do it the other way
	DROP_UNLESS(ipv4_header->time_to_live > 1);
	ipv4_header->time_to_live--;

	// Forward(outputPortName(1))
	return OUTPUT_PORT;
}
