// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <sys/time.h>

#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_byteorder.h>

#include "lib/nat_config.h"
#include "lib/nf_forward.h"
#include "lib/nf_util.h"
#include "lpm/lpm.h"

struct nat_config config;
void *lpm;

void nf_core_init(void)
{
  lpm_init("routing-table.pfx2as", &lpm);
  assert(lpm);
}

int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	struct ipv4_hdr* ip_header = nf_get_mbuf_ipv4_header(mbuf);
	if (ip_header == NULL) {
    return mbuf->port;
	}

	uint8_t dst_device = lpm_lookup(lpm, rte_be_to_cpu_32(ip_header->dst_addr));
        // Concretize the device, to avoid symbolic indexing.
        for (uint8_t d = 0; d < rte_eth_dev_count(); ++d) { if (dst_device == d) { dst_device = d; } }

	// L2 forwarding
	struct ether_hdr* ether_header = nf_get_mbuf_ether_header(mbuf);
	ether_header->s_addr = config.device_macs[dst_device];
	ether_header->d_addr = config.endpoint_macs[dst_device];

	return dst_device;
}

void nf_config_init(int argc, char** argv) {
  nat_config_init(&config, argc, argv);
}

void nf_config_cmdline_print_usage(void) {
  nat_config_cmdline_print_usage();
}

void nf_print_config() {
  nat_print_config(&config);
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_begin(unsigned lcore_id,
                             time_t time) {
}

void nf_add_loop_iteration_assumptions(unsigned lcore_id,
                                       time_t time) {
}

void nf_loop_iteration_end(unsigned lcore_id,
                           time_t time) {
}
#endif

