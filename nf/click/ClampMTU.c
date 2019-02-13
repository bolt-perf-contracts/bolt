#include "click/nf_click.h"


int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	int config_value;
	klee_make_symbolic(&config_value, sizeof(int), "config_value");

	// inputPortName(0)
	DROP_UNLESS(mbuf->port == INPUT_PORT);

	// NOTE: Symnet doesn't seem to ensure the packet is IP before checking MTU...
	struct ipv4_hdr* ipv4_header = nf_get_mbuf_ipv4_header(mbuf);
        DROP_UNLESS(ipv4_header != NULL);

	// Constrain(IPLength,:<=:(ConstantValue(configParams(0).value.toInt)))
	DROP_UNLESS(ipv4_header->total_length <= config_value);

	// Forward(outputPortName(1))
	return OUTPUT_PORT;
}
