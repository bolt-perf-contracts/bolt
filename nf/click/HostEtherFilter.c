#include "click/nf_click.h"


int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	int config_value;
	klee_make_symbolic(&config_value, sizeof(int), "config_value");

	// inputPortName(0)
	DROP_UNLESS(mbuf->port == INPUT_PORT);

	// NOTE: Symnet doesn't seem to ensure the packet is Ethernet before checking ether type...
	struct ether_hdr* ether_header = nf_get_mbuf_ether_header(mbuf);
        DROP_UNLESS(ether_header != NULL);

	// Constrain(Tag("L2")+EtherTypeOffset,:==:(ConstantValue(configParams(0).value.toInt)))
	DROP_UNLESS(ether_header->ether_type == config_value);

	// Forward(outputPortName(0))
	return OUTPUT_PORT;
}
