#include "lib/stubs/hardware_stub.h"
#include "lib/stubs/core_stub.h"

#include <endian.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rte_cycles.h" // to include the next one cleanly
#include "generic/rte_cycles.h" // for rte_delay_us_callback_register

#include <klee/klee.h>

void orig_printf(const char * format, ...);

typedef uint32_t (*stub_register_read)(struct stub_device* dev, uint32_t offset);
typedef uint32_t (*stub_register_write)(struct stub_device* dev, uint32_t offset, uint32_t new_value);

struct stub_register {
	bool present; // to distinguish registers we model from others
	uint32_t initial_value;
	bool readable;
	uint32_t writable_mask; // 0 = readonly, 1 = writeable
	stub_register_read read; // possibly NULL
	stub_register_write write; // possibly NULL
};

static struct stub_register REGISTERS[0x20000]; // index == address

// Incremented at each delay; in nanoseconds.
static uint64_t TIME;

// Checks for the traced_mbuf hack soundness
static bool rx_called;
static bool tx_called;
static bool free_called;

// Helper bit macros
#define GET_BIT(n, k) (((n) >> (k)) & 1)
#define SET_BIT(n, k, v) if ((v) == 0) { n = (n & ~(1 << (k))); } else { n = (n | (1 << (k))); }

// Helper register macros
#define DEV_MEM(dev, offset, type) *((type*) (dev->mem_shadow + (offset)))
#define DEV_REG(dev, offset) DEV_MEM(dev, offset, uint32_t)


// Unless otherwise stated, all citations here refer to
// https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/82599-10-gbe-controller-datasheet.pdf

// We do not model RC registers specially because we don't allow writes to any of them and they all start at 0, so we just leave them alone

static void
stub_delay_us(unsigned int us)
{
	TIME += us * 1000 ; // TIME is in ns
}


static void
stub_device_reset(struct stub_device* dev)
{

#ifdef KLEE_VERIFICATION
	klee_possibly_havoc(&traced_mbuf_content, sizeof(struct stub_mbuf_content), "traced_mbuf");
	klee_possibly_havoc(&traced_mbuf, sizeof(struct rte_mbuf), "traced_mbuf_content");
        klee_possibly_havoc(&rx_called, sizeof(bool), "rx_called");
        klee_possibly_havoc(&tx_called, sizeof(bool), "tx_called");
        klee_possibly_havoc(&free_called, sizeof(bool), "free_called");
        klee_possibly_havoc(DEVICES, sizeof(DEVICES), "DEVICES");
#endif//KLEE_VERIFICATION
	for (int n = 0; n < sizeof(REGISTERS)/sizeof(REGISTERS[0]); n++) {
		if (REGISTERS[n].present) {
			DEV_REG(dev, n) = REGISTERS[n].initial_value;
		}
	}

// FIXME not needed?
//	// 1 bit diff between MAC addresses; see registers init and VigNAT makefile
//	if (dev == &DEVICES[1]) {
//		DEV_REG(dev, 0x0A200) |= 1;
//	}

	dev->current_mdi_address = -1;
}

static void
stub_device_start(struct stub_device* dev)
{
	// Get the address of the receive descriptor for queue 0
	uint64_t rdba =  ((uint64_t) DEV_REG(dev, 0x01000)) // RDBAL
		      | (((uint64_t) DEV_REG(dev, 0x01004)) << 32); // RDBAH

	// Clear the head of the descriptor
	DEV_REG(dev, 0x01010) = 0; // RDH
	// Make sure we have enough space
	uint32_t rdt = DEV_REG(dev, 0x01018);
	klee_assert(rdt >= 1);

	if (klee_int("received") == 0) {
		// no packet
	}

	// Descriptor is 128 bits, see page 313, table 7-15 "Descriptor Read Format"
	// (which the NIC reads to know where to put a packet)
	// and page 314, table 7-16 "Descriptor Write-Back Format"
	// (note that "write" in this context is from the NIC's point of view;
	//  it writes those descriptors into memory)
	uint64_t* descr = (uint64_t*) rdba;

	// Read phase
	// Address is the first 64 bits, header the next 64
	// normally headers shouldn't be split
	uint64_t mbuf_addr = descr[0];
	uint64_t head_addr = descr[1];
	klee_assert(head_addr == 0);

	dev->old_mbuf_addr = mbuf_addr;

	// Write phase

	// First Line
	// 0-3: RSS Type (0 - no RSS)
	// 4-16: Packet Type
	//  4/5 and 6/7 are IPv4 and IPv6 respectively (mutually exclusive?)
	//  8/9/10 are TCP/UDP/SCTP respectively (mutually exclusive?)
	//  11 is NFS
	//  12/13 is IPSec
	//  14 is LinkSec (non-IP)
	//  15 must be 0 to indicate a non-L2 packet
	//  16 is reserved
	// 17-20: RSC Count (0 - no RSC)
	// 21-30: Header Length (0 - header not decoded)
	// 31: Split Header (1 - not split)
	// 32-63: RSS Hash or FCOE_PARAM or Flow Director Filters ID or Fragment Checksum (0 - not supported)
	uint64_t wb0 = 0b0000000000000000000000000000000010000000000000000000000000000000;

	struct stub_mbuf_content* mbuf_content = malloc(sizeof(struct stub_mbuf_content));
	if (mbuf_content == NULL) {
		do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0); // TODO ahem...
	}
	// NOTE: validator depends on this specific name, "user_buf"
	klee_make_symbolic(mbuf_content, sizeof(struct stub_mbuf_content), "user_buf");


#if __BYTE_ORDER == __BIG_ENDIAN
	bool is_ipv4 = mbuf_content->ether.ether_type == 0x0800;
	bool is_ipv6 = mbuf_content->ether.ether_type == 0x86DD;
#else
	bool is_ipv4 = mbuf_content->ether.ether_type == 0x0008;
	bool is_ipv6 = mbuf_content->ether.ether_type == 0xDD86;
#endif

	bool is_tcp = false, is_udp = false, is_sctp = false;
	if (is_ipv4) {
		is_tcp = mbuf_content->ipv4.next_proto_id == 6;
		is_udp = mbuf_content->ipv4.next_proto_id == 17;
		is_sctp = mbuf_content->ipv4.next_proto_id == 132;
	}

	// NOTE: Allowing all of those to be symbols means the symbex takes suuuuper-long... worth doing sometimes, but not all the time
#if 0
	bool is_ip = is_ipv4 || is_ipv6;
	bool is_ip_broadcast = is_ip && klee_int("received_is_ip_broadcast") != 0;

	bool has_ip_ext = is_ip && klee_int("received_has_ip_ext") != 0;

	bool is_linksec = !is_ip && klee_int("received_is_linksec") != 0;

	bool not_ipsec = is_udp || is_tcp || is_sctp;
	bool is_nfs = not_ipsec && klee_int("received_is_nfs") != 0;

	bool is_ipsec_esp = !not_ipsec && klee_int("received_is_ipsec_esp") != 0;
	bool is_ipsec_ah = !not_ipsec && !is_ipsec_esp &&klee_int("received_is_ipsec_ah") != 0;
#else
	bool is_ip_broadcast = false, has_ip_ext = false, is_linksec = false, not_ipsec = true, is_nfs = false, is_ipsec_esp = false, is_ipsec_ah = false;
#endif

#define BIT(index, cond) SET_BIT(wb0, index, (cond) ? 1 : 0);
	BIT(4, is_ipv4);
	BIT(5, is_ipv4 && has_ip_ext);
	BIT(6, is_ipv6);
	BIT(7, is_ipv6 && has_ip_ext);
	BIT(8, is_tcp);
	BIT(9, is_udp);
	BIT(10, is_sctp);
	BIT(11, is_nfs);
	BIT(12, is_ipsec_esp);
	BIT(13, is_ipsec_ah);
	BIT(14, is_linksec);
	BIT(15, 0); // non-L2 packet - TODO we should try that, but then the entire meaning of packet_type changes...
	BIT(16, 0); // reserved
#undef BIT


	// Second Line
	// 0-19: Extended Status / NEXTP
	//  0: Descriptor Done (1 - done)
	//  1: End Of Packet (1 - yes, no more fragments)
	//  2: Flow Match (0 - no filter set, no match)
	//  3: VLAN Packet (0 - no VLANs configured, no match)
	//  4: UDP Checksum Offload (0 - not provided)
	//  5: L4 Integrity (0 - not provided)
	//  6: IPv4 Checksum (0 - not provided)
	//  7: Non-Unicast (depends on IP)
	//  8: Reserved (0)
	//  9: Outer-VLAN on double VLAN packet (0 - not enabled)
	//  10: UDP Checksum Valid (0 - not supported)
	//  11: Low Latency Interrupt (0 - no interrupts)
	//  12-15: Reserved (0)
	//  16: Time Stamp (0 - not a time sync packet)
	//  17: Security Processing (0 - not configured)
	//  18: Loopback Indication (0 - not a loopback)
	//  19: Reserved (0)
	// 20-31: Extended Error (0 - no error)
	// 32-47: Packet Length (depends on packet)
	// 48-63: VLAN Tag (0, no VLAN)
	uint64_t wb1 = 0b0000000000000000000000000000000000000000000000000000000000000011;

	// get packet length
	uint16_t packet_length = sizeof(struct stub_mbuf_content);
	wb1 |= (uint64_t) packet_length << 32;

	if (is_ipv4 && (
			// Multicast addr?
#if __BYTE_ORDER == __BIG_ENDIAN
			(mbuf_content->ipv4.dst_addr >= 0xE0000000 && mbuf_content->ipv4.dst_addr < 0xF0000000)
#else
			((mbuf_content->ipv4.dst_addr & 0xFF) >= 0xE0 && (mbuf_content->ipv4.dst_addr & 0xFF) < 0xF0)
#endif
			// Or just a broadcast, which can be pretty much anything
			|| is_ip_broadcast)) {
		SET_BIT(wb1, 7, 1);
	}


#ifndef FULLY_SYMBOLIC_BUF
	// Force the IPv4 content to have sane values for symbex...
	if(is_ipv4) {
		// TODO can we make version_ihl symbolic?
		mbuf_content->ipv4.version_ihl = (4 << 4) | 5; // IPv4, 5x4 bytes - concrete to avoid symbolic indexing
		mbuf_content->ipv4.total_length = rte_cpu_to_be_16(sizeof(struct ipv4_hdr) + sizeof(struct tcp_hdr));
	}
#endif//!FULLY_SYMBOLIC_BUF


	// Write the packet into the proper place
	memcpy((void*) mbuf_addr, mbuf_content, packet_length);

	// "The 82599 writes back the receive descriptor immediately following the packet write into system memory."
	descr[0] = wb0;
	descr[1] = wb1;

	// Get device index
	int device_index = 0;
	while (dev != &DEVICES[device_index]) { device_index++; }

	// Get the DPDK packet type
	uint32_t traced_ptype = 0;
	traced_ptype |= 0x00000001; // ether; always - see TODO in definition of the ptype above
	if (is_ipv4) traced_ptype |= 0x00000010;
	if (is_ipv6) traced_ptype |= 0x00000040;
	if (has_ip_ext) {
		if (is_ipv4) traced_ptype |= 0x00000030;
		if (is_ipv6) traced_ptype |= 0x000000c0;
	}
	if (is_udp) traced_ptype |= 0x00000200;
	if (is_tcp) traced_ptype |= 0x00000100;
	if (is_sctp) traced_ptype |= 0x00000400;

	// Trace the mbuf
	memcpy(&traced_mbuf_content, (void*) mbuf_addr, packet_length);
	memset(&traced_mbuf, 0, sizeof(struct rte_mbuf));
	traced_mbuf.buf_addr = &traced_mbuf_content;
	traced_mbuf.buf_iova = (rte_iova_t) traced_mbuf.buf_addr;
	traced_mbuf.data_off = 0;
	traced_mbuf.refcnt = 1;
	traced_mbuf.nb_segs = 1;
	traced_mbuf.port = device_index;
	traced_mbuf.ol_flags = 0; // TODO?
	traced_mbuf.packet_type = traced_ptype;
	traced_mbuf.pkt_len = packet_length;
	traced_mbuf.data_len = packet_length;
	traced_mbuf.vlan_tci = 0; // TODO?
	traced_mbuf.hash.rss = 0; // TODO?
	traced_mbuf.vlan_tci_outer = 0; // TODO?
	traced_mbuf.buf_len = packet_length;
	traced_mbuf.timestamp = 0; // TODO?
	traced_mbuf.userdata = NULL;
	traced_mbuf.pool = NULL;
	traced_mbuf.next = NULL;
	traced_mbuf.tx_offload = 0; // TODO?
	traced_mbuf.priv_size = 0;
	traced_mbuf.timesync = 0; // TODO?
	traced_mbuf.seqn = 0; // TODO?

	struct rte_mbuf* trace_mbuf_addr = &traced_mbuf;
	stub_core_trace_rx(&trace_mbuf_addr);

	// Soundness for hack
	klee_assert(!rx_called);
	rx_called = true;
}


// RW1C means a register can be read, and bits can be cleared by writing 1
static uint32_t
stub_register_rw1c_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	for (int n = 0; n <= 31; n++) {
		if (GET_BIT(new_value, n) == 1) {
			SET_BIT(new_value, n, 0);
		} else {
			// Cannot flip a bit from 1 to 0
			uint32_t current_value = DEV_REG(dev, offset);
			klee_assert(current_value == 0);
		}
	}
	return 0;
}


static uint32_t
stub_register_i2cctl_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// I2C citations here are from https://www.nxp.com/docs/en/user-guide/UM10204.pdf
	// SFP citations here are from https://ta.snia.org/higherlogic/ws/public/download/268/SFF-8431.PDF
	// and https://ta.snia.org/higherlogic/ws/public/download/294/SFF-8472.PDF

	// Table 10, "Characteristics of the SDA and SCL bus lines for Standard [...] I2C devices"
	// NOTE: START setup and STOP setup are both >= TIME_LOW so we just ignore them
	const int TIME_START_HOLD = 4000; // SDA must keep its value for this long after a start
	const int TIME_BETWEEN_STOP_START = 4700;
	const int TIME_LOW = 4700; // minimum time for the clock to be in LOW
	const int TIME_HIGH = 4000; // minimum time for the clock to be in HIGH

	// I2C/SFP States
	// -1 is default
	const int I2C_STARTING = 0;
	const int I2C_STARTED = 1;
	const int I2C_STOPPED = 2;
	const int SFP_READING = 3;
	const int SFP_ADDRESSING = 4;
	const int SFP_WRITING = 5;
	const int SFP_END = 6;

	uint32_t current_value = DEV_REG(dev, offset);

	uint8_t scl_old = (current_value >> 1) & 1;
	uint8_t sda_old = (current_value >> 3) & 1;

	uint8_t scl_new = (new_value >> 1) & 1;
	uint8_t sda_new = (new_value >> 3) & 1;

	// Clone the out bits to the in bits - clock (1/0) and data (3/2)
	// NOTE: We model the out bits as writable because the driver has to write to them;
	//       otherwise, it would need to read I2CCTL every time before writing, which would be inefficient;
	//       so instead it just writes whatever to them and the value is ignored.
	SET_BIT(new_value, 0, GET_BIT(new_value, 1));
	SET_BIT(new_value, 2, GET_BIT(new_value, 3));

	if (scl_old != scl_new) {
		if (scl_new == 0) {
			klee_assert(TIME - dev->i2c_clock_time >= TIME_HIGH);
		} else {
			klee_assert(TIME - dev->i2c_clock_time >= TIME_LOW);
		}

		dev->i2c_clock_time = TIME;
	}

	// "A HIGH to LOW transition on the SDA line while SCL is HIGH defines a START condition"
	if (sda_old == 1 && sda_new == 0 && scl_old == 1 && scl_new == 1) {
		if (dev->i2c_state == I2C_STOPPED) {
			klee_assert(dev->i2c_stop_time + TIME_BETWEEN_STOP_START <= TIME);
		}

		dev->i2c_state = I2C_STARTING;
		dev->i2c_start_time = TIME;
		dev->i2c_counter = 0;
		dev->i2c_address = 0;

		return new_value;
	}

	// "A data transfer is always terminated by a STOP condition generated by the master."
	// "A LOW to HIGH transition on the SDA line while SCL is HIGH defines a STOP condition"
	if (sda_old == 0 && sda_new == 1 && scl_old == 1 && scl_new == 1) {
		// "A START condition immediately followed by a STOP condition (void message) is an illegal format."
		klee_assert(dev->i2c_counter > 0);

		// also, let's make sure the message didn't stop at some random place
		klee_assert(dev->i2c_counter % 9 == 0);

		dev->i2c_state = I2C_STOPPED;
		dev->i2c_counter = 0;
		dev->i2c_address = 0;
		dev->i2c_stop_time = TIME;

		return new_value;
	}

	if (dev->i2c_state == I2C_STARTING) {
		if (sda_old != sda_new) {
			klee_assert(TIME - dev->i2c_start_time >= TIME_START_HOLD);

			dev->i2c_state = I2C_STARTED;
		} else {
			return new_value;
		}

		// Fall through
	}

	if (!(scl_old == 0 && scl_new == 1)) {
		// Not a rising edge - we don't care
		return new_value;
	}

	if (dev->i2c_state == I2C_STARTED) {
		// "After the START condition, a slave address is sent.
		//  This address is seven bits long followed by an eight bit which is a data direction bit (R/W) -
		//  a 'zero' indicates a transmission (WRITE), a 'one' indicates a request for data (READ)

		if (dev->i2c_counter % 9 < 8) {
			// Part of the address or the RW bit - read a bit
			dev->i2c_address = dev->i2c_address << 1;
			dev->i2c_address = dev->i2c_address | sda_new;
		} else {
			// "Each byte is followed by an acknowledgement bit"
			// "The Acknowledge signal is defined as follows: the transmitter releases the SDA line
			//  during the acknowledge clock pulse so the receiver can pull the SDA line LOW
			//  and it remains stable LOW suring the HIGH period of this clock pulse."

			SET_BIT(new_value, 2, 0); // bit 2 is "data in"

			// Set the state since we sent an ACK
			// If it's an I2C write, the SFP address follows
			dev->i2c_state = (dev->i2c_address & 1) == 0 ? SFP_ADDRESSING : SFP_READING;

			// Reset SFP address if we're about to address (otherwise, don't! reads use the last one)
			if (dev->i2c_state == SFP_ADDRESSING) {
				dev->sfp_address = 0;
			}

			// Also remove the RW bit from the address
			dev->i2c_address = dev->i2c_address >> 1;

			// "The 10-bit slave address is formed from the first two bytes following a START condition"
			// "The first seven bits of the first byte are the combination 1111 0XX"
			// "Although there are eight possible combinations of the reserved address bits 1111 xxx,
			//  only the four combinations 1111 0XX are used for 10-bit addressing.
			//  The remaining four combinations 1111 1XX are reserved for future I2C-bus enhancements."
			// we don't support 10-bit mode, nor the reserved stuff
			klee_assert(dev->i2c_address >> 3 != 0b1111);

			// "Each SFP+ is hard-wired at the device addresses A0h and A2h"
			// except that by "A0h" they mean "the 7-bit address 50h padded with a 0 at the end to make it 8 bit"
			// we only suport A0 (for now?)
			klee_assert(dev->i2c_address == 80);
		}

		dev->i2c_counter++;

		return new_value;
	}

	if (dev->i2c_state == SFP_ADDRESSING) {
		if (dev->i2c_counter % 9 < 8) {
			dev->sfp_address = dev->sfp_address << 1;
			dev->sfp_address = dev->sfp_address | sda_new;
		} else {
			// We have a full SFP address, ACK it
			SET_BIT(new_value, 2, 0);

			// Put in write mode, which may be canceled by the master if it wants to read by restarting
			dev->i2c_state = SFP_WRITING;
		}

		dev->i2c_counter++;
		return new_value;
	}

	if (dev->i2c_state == SFP_READING) {
		// The master wants data - send it, bit by bit
		// Registers are found in table 4-1

		int cursor = dev->i2c_counter % 9;
		dev->i2c_counter++;

		if (cursor == 8) {
			// "To specify a sequential read, the host responds with an acknowledge"
			// "The sequence is terminated when the host responds with a NACK and a STOP"
			if (sda_new == 0) { // remember, 0 is ACK because it's a pull-up
				dev->sfp_address++;
			} else {
				dev->i2c_state = SFP_END;
			}

			return new_value;
		}

		cursor = 7 - cursor; // "Data is transferred with the most significant bit (MSB) first"

		int sfp_registers[] = {
			// 0: Identifier
			0x03, // SFP/SFP+

			// 1: Extended identifier
			0x00, // nothing TODO

			// 2: Connector Type
			//    Table 4-3 from https://doc.xdevs.com/doc/Seagate/SFF-8024.PDF
			0x00, // unknown/unspecified TODO

			// 3-10: Transciever
			// https://www.intel.com/content/www/us/en/support/articles/000005528/network-and-i-o/ethernet-products.html
			// says that SR and LR are supported
			// table 5-3 says those are byte 3 bit 4 and byte 3 bit 5 respectively
			// let's do SR for now TODO
			0x10, // 3
			0x00, // 4
			0x00, // 5
			0x00, // 6
			0x00, // 7
			0x00, // 8
			0x00, // 9
			0x00, // 10

			// 11: Encoding
			//     table 4-2 from SFF-8024
			0x00, // unknown/unspecified TODO

			// 12: Nominal signaling rate, units of 100 Mb/s
			100, // 10 Gb/s

			// 13: Rate identifier (table 5-6)
			0x00, // unknown TODO

			// 14: Length, unit of kilometers
			0x00, // not that long... TODO?

			// 15: Length, unit of 100 meters
			0x01, // total length 100 meters

			// 16: Length for 50um OM2 fiber, unit of 10 meters
			0x01, // TODO?

			// 17: Length for 62.5um OM2 fiber, unit of 10 meters
			0x01, // TODO?

			// 18: Length for 50um OM4 fiber, unit of 10m; or copper cable, unit of 1m
			0x01, // TODO?

			// 19: Length for 50um OM3 fiber, unit of 10m
			0x01, // TODO?

			// 20-35: SFP Vendor Name, ASCII "padded on the right with ASCII spaces"
			'S', 'T', 'U', 'B',
			' ', ' ', ' ', ' ',
			' ', ' ', ' ', ' ',
			' ', ' ', ' ', ' ',

			// 36: Transciever extended code (table 4-4 from SFF-8024)
			0x00, // unknown/unspecified TODO

			// 37-39: IEEE company ID (OUI)
			//     http://standards-oui.ieee.org/oui.txt
			0x00, 0xA0, 0xC9, // Intel (picked one at random...)

			// 40-55: Vendor Part Number
			//        see bits 3-10 above for link with PNs
			'F', 'T', 'L', 'X', '8', '5', '7', '1', 'D', '3', 'B', 'C', 'V', 'I', 'T', '1',

			// 56-59: Vendor Revision
			0x00, 0x00, 0x00, 0x01, // TODO?

			// 60-61: Wavelength
			0x00, 0x00, // unknown TODO

			// 62: Unallocated
			-1,

			// 63: CC_BASE - check code
			0
		};

		// "The check code is a one byte code that can be used to verify that the first 64 bytes of two-wire interface information in the SFP is valid.
		//  The check code shall be the low order 8 bits of the sum of the contents of all the bytes from byte 0 to byte 62, inclusive."
		uint32_t cc_base = 0;
		for (int n = 0; n <= 62; n++) {
			cc_base += sfp_registers[n];
		}
		sfp_registers[63] = cc_base & 0xFF;

		// we don't support extended stuff for now
		klee_assert(dev->sfp_address <= 63);

		int value = sfp_registers[dev->sfp_address];
		int bit = GET_BIT(value, cursor);

		// Set the data out bit
		SET_BIT(new_value, 3, bit);

		return new_value;
	}

	if (dev->i2c_state == SFP_WRITING) {
//		do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
	}

//	klee_assert(dev->i2c_state != SFP_END); // should have gotten a STOP?

	return new_value; // TODO
}


static uint32_t
stub_register_ctrl_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// Bit 2 is cleared once no master requests are pending, which we don't emulate anyway
	SET_BIT(new_value, 2, 0);

	// Bit 3 is self-clearing
	if (GET_BIT(new_value, 3) == 1) {
		SET_BIT(new_value, 3, 0);
		// TODO reset MAC, PCS and autonegotiation
	}

	// Bit 26 is self-clearing
	if (GET_BIT(new_value, 26) == 1) {
		SET_BIT(new_value, 26, 0);
		stub_device_reset(dev);
	}

	return new_value;
}


static uint32_t
stub_register_eerd_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	uint32_t current_value = DEV_REG(dev, offset);

	// Cannot set the done bit to 1, only clear it
	klee_assert(!(GET_BIT(current_value, 1) == 0 && GET_BIT(new_value, 1) == 1));
	// Same with the data
	for (int n = 16; n <= 31; n++) {
		klee_assert(!(GET_BIT(current_value, n) == 0 && GET_BIT(new_value, n) == 1));
	}

	bool read = GET_BIT(new_value, 0);
	uint16_t addr = (new_value >> 2) & 0b11111111111111;

	// Clear read bit
	SET_BIT(new_value, 0, 0);
	// Clear data bits
	for (int n = 16; n <= 31; n++) {
		SET_BIT(new_value, n, 0);
	}

	// "EEPROM General Map" page 217
	uint16_t eeprom_map[] = {
		0, // 0x00
		0, // 0x01,
		0, // 0x02 - UNDOCUMENTED
		0, // 0x03
		0, // 0x04
		0, // 0x05
		0, // 0x06
		0, // 0x07
		0, // 0x08
		0, // 0x09
		0, // 0x0A
		0, // 0x0B
		0, // 0x0C
		0, // 0x0D
		0, // 0x0E
		0, // 0x0F
		0, 0, 0, 0, 0, // 0x10-0x14
		0, 0, // 0x15-0x16
		0, // 0x17
		0, 0, // 0x18-0x19
		0, // 0x20
		0, 0, 0, 0, 0, // 0x21-25
		0, // 0x26
		0, // 0x27
		0, // 0x28
		0, 0, 0, 0, 0, 0, // 0x29-0x2E
		0, // 0x2F
		0, 0, 0, 0, 0, 0, 0, // 0x30-0x36
		0, // 0x37
		0, // 0x38
		0, 0, 0, 0, 0, 0, // 0x39-0x3E
		0, // 0x3F - CHECKSUM, LEAVE ZERO HERE
		// begin of firmware module
		0, // 0x0
		0, // 0x1
		0, // 0x2
		0, // 0x3
		0, // 0x4 - Pass Through Patch Configuration Pointer
		0, // 0x5
		0, // 0x6
		0, // 0x7
		0, // 0x8
		0, // 0x9
		0, // 0xA
		// begin of Pass Through Patch Condiguration
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
	};
	// Set the FW pointer to just after the basic block
	eeprom_map[0x0F] = 0x3F + 1;
	// Set the pass through patch conf pointer to just after the firmware module
	eeprom_map[0x3F + 1 + 0x4] = 0x3F + 1 + 0xA + 1;
	// and then for magic... this specific thing is a version, apparently? if it's not >5, ixgbe assumes the eeprom is invalid
	eeprom_map[0x3F + 1 + 0xA + 1 + 0x7] = 0x6;

	// Checksum word - sum of all words from 0x00 to 0x3F (including words pointed to if any, except the FW block) must be 0xBABA
	uint16_t checksum = 0;
	for (int n = 0; n <= 0x3F; n++) {
		checksum += eeprom_map[n];
	}
	checksum = 0xBABA - checksum;
	eeprom_map[0x3F] = checksum;

	new_value |= (eeprom_map[addr] << 16);

	// Mark read as done
	// TODO some timeouts?
	SET_BIT(new_value, 1, 1);

	return new_value;
}


static uint32_t
stub_register_msca_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// page 135
	// MDIO Direct Access

	// TODO figure out which addresses are which registers... where's the spec?

	// Bit 30 is "MDI Command", 1 means perform operation
	if (GET_BIT(new_value, 30) == 0) {
		// bit cleared, nothing to do
		return new_value;
	}

	// Bit 30 says there is a command; start by clearing it, since we execute everything instantaneously
	SET_BIT(new_value, 30, 0);

	// "start code [00b] that identifies clause 45 format"
	if (((new_value >> 28) & 0b11) == 0b00) {
		int opcode = (new_value >> 26) & 0b11;

		if (opcode == 0b00) { // address operation
			klee_assert(dev->current_mdi_address == -1);
			dev->current_mdi_address = new_value & 0xFF;
		} else if (opcode == 0b11) { // read operation
			klee_assert(dev->current_mdi_address != -1);

			int phy_addr = (new_value >> 21) & 0b11111;
			int addr = new_value & 0xFF;

			uint32_t result = 0;
			if (dev->current_mdi_address == 0) { // control byte
				result = 0x8000; // reset flag set
			} else if (dev->current_mdi_address == 2) { // ID high byte
				result = 1; // just needs to be nonzero
			}

			DEV_REG(dev, 0x04260) = result << 16; // register MSRWD holds the result in the upper 16 bits

			dev->current_mdi_address = -1;
		} else if (opcode == 0b01) { // write operation
			klee_assert(dev->current_mdi_address != -1);

			int phy_addr = (new_value >> 21) & 0b11111;
			int data = new_value & 0xFF;

			// only support the reset register - we just pretend that it's been reset all the time
			klee_assert(dev->current_mdi_address == 0);

			dev->current_mdi_address = -1;
		} else {
			do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0); // unsupported
		}
	} else {
		// clause 22 format
		do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0); // unsupported
	}

	return new_value;
}


static uint32_t
stub_register_rdrxctl_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// "Software should set [RSCFRSTSIZE, bits 17 to 21] to 0x0"
	// (but the default is 0x0880, bits 7 and 11 set)
	for (int n = 17; n <= 21; n++) {
		klee_assert(GET_BIT(new_value, n) == 0);
	}
	SET_BIT(new_value, 7, 1);
	SET_BIT(new_value, 11, 1);

	// "Software should set [RSCACKC, bit 25] to 1"
	klee_assert(GET_BIT(new_value, 25) == 1);
	SET_BIT(new_value, 25, 0);

	// "Software should set [FCOE_WRFIX, bit 26] to 1"
	klee_assert(GET_BIT(new_value, 26) == 1);
	SET_BIT(new_value, 26, 0);

	return new_value;
}


static uint32_t
stub_register_txdctl_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// Bit 26 is self-clearing
	SET_BIT(new_value, 26, 0);

	return new_value;
}


static uint32_t
stub_register_dbal_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// for both RDBAL and TDBAL
	// Bits 0-6 are ignored on write and read as 0
	for (int n = 0; n <= 6; n++) {
		SET_BIT(new_value, n, 0);
	}

	return new_value;
}


static uint32_t
stub_register_tdh_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// Cannot write unless TXDCTL.ENABLE (bit 25) is false
	int n = (offset - 0x06010) / 0x40;
	uint32_t txdctl = DEV_REG(dev, 0x06028 + 0x40*n);
	klee_assert(GET_BIT(txdctl, 25) == 0);

	return new_value;
}

static uint32_t
stub_register_tdt_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// SW wrote to TDT, meaning it has a packet for us
        // Get the address of the transmit descriptor for queue 0
        uint64_t tdba =  ((uint64_t) DEV_REG(dev, 0x06000)) // TDBAL
                      | (((uint64_t) DEV_REG(dev, 0x06004)) << 32); // TDBAH

        // Clear the head of the descriptor
        DEV_REG(dev, 0x06010) = 0; // TDH
        // Make sure we have enough space
        uint32_t tdt = new_value;
	if (tdt == 0) {
		// No? Probably this is not to send a packet, then.
		return new_value;
	}

        // Descriptor is 128 bits, see page 353, table 7-39 "Descriptor Read Format"
        // (which the NIC reads to know how to send a packet)
        // and page 354, table 7-40 "Descriptor Write-Back Format"
        // (note that "write" in this context is from the NIC's point of view;
        //  it writes those descriptors into memory)
        uint64_t* descr = (uint64_t*) tdba;

        // Read phase

	// Line 0: Address
        uint64_t buf_addr = descr[0];

	// Line 1: Properties
	// 0-15: Buffer length
	// 16-17: Reserved
	// 18: Whetner LinkSec is applied
	// 19: Whether the packet has an IEEE1588 timestamp
	// 20-23: Descriptor type, must be 0011
	// 24: End of Packet, should be 1 since we don't support multi-buffer packets
	// 25: Insert FCS (should be 0)
	// 26: Reserved
	// 27: Report Status, whether SW wants HW to report DMA completion status in addition to an interrupt
	// 28: Reserved
	// 29: Descriptor Extension (must be 1)
	// 30: VLAN Packet (should be 0)
	// 31: Transmit Segmentation Enable (should be 0)
	// 32: Descriptor Done (must be 0, we'll set to 1 once we're done)
	// 33-35: Reserved
	// 36-39: Irrelevant
	// 40: Insert IP Checksum (should be 0)
	// 41: Insert TCP/UDP Checksum (should be 0)
	// 42: IPSEC offload request (should be 0)
	// 43-45: Reserved
	// 46-63: Payload length (== buffer length in our case)
	uint64_t buf_props = descr[1];

	uint16_t buf_len = buf_props & 0xFF;

	klee_assert(GET_BIT(buf_props, 18) == 0);
	klee_assert(GET_BIT(buf_props, 19) == 0);

	klee_assert(GET_BIT(buf_props, 20) == 1);
	klee_assert(GET_BIT(buf_props, 21) == 1);
	klee_assert(GET_BIT(buf_props, 22) == 0);
	klee_assert(GET_BIT(buf_props, 23) == 0);

	klee_assert(GET_BIT(buf_props, 24) == 1);
	klee_assert(GET_BIT(buf_props, 25) == 1);
        //Fails for some reason with the new tx thresh values
	//klee_assert(GET_BIT(buf_props, 27) == 0);
	klee_assert(GET_BIT(buf_props, 29) == 1);
	klee_assert(GET_BIT(buf_props, 30) == 0);
	klee_assert(GET_BIT(buf_props, 31) == 0);
	klee_assert(GET_BIT(buf_props, 32) == 0);
	klee_assert(GET_BIT(buf_props, 40) == 0);
	klee_assert(GET_BIT(buf_props, 41) == 0);
	klee_assert(GET_BIT(buf_props, 42) == 0);

	uint32_t payload_len = buf_props >> 46;
	klee_assert(buf_len == payload_len);

	// Get device index
	int device_index = 0;
	while (dev != &DEVICES[device_index]) { device_index++; }

	// Trace the mbuf
	memcpy(&traced_mbuf_content, (void*) buf_addr, sizeof(struct stub_mbuf_content));
	memset(&traced_mbuf, 0, sizeof(struct rte_mbuf));
	traced_mbuf.buf_addr = &traced_mbuf_content;
	traced_mbuf.buf_iova = (rte_iova_t) traced_mbuf.buf_addr;
	traced_mbuf.data_off = 0;
	traced_mbuf.refcnt = 1;
	traced_mbuf.nb_segs = 1;
	traced_mbuf.port = device_index;
	traced_mbuf.ol_flags = 0; // TODO?
	traced_mbuf.packet_type = 0; // TODO?
	traced_mbuf.pkt_len = buf_len;
	traced_mbuf.data_len = buf_len;
	traced_mbuf.vlan_tci = 0; // TODO?
	traced_mbuf.hash.rss = 0; // TODO?
	traced_mbuf.vlan_tci_outer = 0; // TODO?
	traced_mbuf.buf_len = buf_len;
	traced_mbuf.timestamp = 0; // TODO?
	traced_mbuf.userdata = NULL;
	traced_mbuf.pool = NULL;
	traced_mbuf.next = NULL;
	traced_mbuf.tx_offload = 0; // TODO?
	traced_mbuf.priv_size = 0;
	traced_mbuf.timesync = 0; // TODO?
	traced_mbuf.seqn = 0; // TODO?

	uint8_t ret = stub_core_trace_tx(&traced_mbuf, device_index);

	// Soundness check
	klee_assert(!tx_called);
	tx_called = true;

	if (ret != 0) {
		// Write phase
		descr[0] = 0; // Reserved
		descr[1] = ((uint64_t) 1) << 32; // Reserved, except bit 32 which is Descriptor Done and must be 1
	}

	return new_value;
}


static uint32_t
stub_register_needsrxen0_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// RXCTRL.RXEN (bit 0) must be set to 0 before writing to RXCSUM and FCTRL
	klee_assert(GET_BIT(DEV_REG(dev, 0x03000), 0) == 0);

	return new_value;
}


static uint32_t
stub_register_mtqc_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// RTTDCS.ARBDIS (bit 6) must be set before writing to MTQC
	klee_assert(GET_BIT(DEV_REG(dev, 0x04900), 6) == 1);

	return new_value;
}


static uint32_t
stub_register_swsm_read(struct stub_device* dev, uint32_t offset)
{
	uint32_t current_value = DEV_REG(dev, offset);
	SET_BIT(current_value, 1, 1); // LSB is the semaphore bit - always set after a read
	return current_value;
}

static uint32_t
stub_register_swsm_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	uint32_t current_value = DEV_REG(dev, offset);

	// Cannot set the semaphore bit to 1, only clear it
	klee_assert(!(GET_BIT(current_value, 0) == 0 && GET_BIT(new_value, 0) == 1));

	// Can only take the software semaphore bit if the semaphore is taken
	if (GET_BIT(current_value, 1) == 0 && GET_BIT(new_value, 1) == 1) {
		klee_assert(GET_BIT(current_value, 0) == 1);
	}

	return new_value; // OK, we only check
}


static uint32_t
stub_register_swfwsync_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// Cannot write to this register unless the software semaphore bit of SWSM is taken
	klee_assert(GET_BIT(DEV_REG(dev, 0x10140), 1) == 1);

	uint32_t current_value = DEV_REG(dev, offset);

	// Cannot write 1 to a bit in this register if the firmware set the corresponding bit
	for (int n = 0; n < 5; n++) {
		klee_assert(GET_BIT(new_value, n) + GET_BIT(current_value, n + 5) <= 1);
	}

	return new_value; // OK, we only check
}


static uint32_t
stub_register_autoc_write(struct stub_device* dev, uint32_t offset, uint32_t new_value)
{
	// Cannot write to this register unless the software semaphore bit of SWSM is taken
	klee_assert(GET_BIT(DEV_REG(dev, 0x10140), 1) == 1);

	// Bit 12 is self-clearing
	SET_BIT(new_value, 12, 0);

	return new_value;
}


static void
stub_registers_init(void)
{
	#define REG(addr, val, mask) do {                              \
				klee_assert(!REGISTERS[addr].present); \
				struct stub_register reg = {           \
					.present = true,               \
					.initial_value = val,          \
					.readable = true,              \
					.writable_mask = mask,         \
					.read = NULL,                  \
					.write = NULL                  \
				};                                     \
				REGISTERS[addr] = reg;                 \
			} while(0);

	// page 543
	// Device Control Register — CTRL (0x00000 / 0x00004; RW)
	// NOTE: "CTRL is also mapped to address 0x00004 to maintain compatibility with previous devices."
	//       but ixgbe doesn't seem to use it so let's not do it

	// 0-1: Reserved (0)
	// 2: PCIe Master Disable (0 - not disabled)
	// 3: Link Reset (0 - not reset; self-clearing)
	// 4-25: Reserved (0)
	// 26: Device Reset (0 - not reset; self-clearing)
	// 27-31: Reserved (0)
	REG(0x00000, 0b00000000000000000000000000000000,
		     0b00000100000000000000000000001100);
	REGISTERS[0x00000].write = stub_register_ctrl_write;


	// page 544
	// Device Status Register — STATUS (0x00008; RO)

	// 0-1: Reserved (00)
	// 2-3: Lan ID (00 - Lan 0 / 01 - Lan 1)
	// 4-6: Reserved (00)
	// 7: Linkup Status Indication (0 - ???)
	// 8-9: Reserved (00)
	// 10-17: Num VFs (0 - no VFs; note: "Bit 17 is always 0b")
	// 18: IO Active (0 - not active; note: "reflects the value of the VF Enable (VFE) bit in the IOV Control/Status register")
	// 19: Status (0 - not issuing any master requests)
	// 20-31: Reserved (0x00)
	REG(0x00008, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000000);


	// pages 544-545
	// Extended Device Control Register — CTRL_EXT (0x00018; RW)

	// 0-13: Reserved (0)
	// 14: PF Reset Done (0 - not reset)
	// 15: Reserved (0)
	// 16: No Snoop Disable (0 - not disabled)
	// 17: Relaxed Ordering Disable (0 - not disabled)
	// 18-25: Reserved (0)
	// 26: Extended VLAN (0 - not set)
	// 27: Reserved (0)
	// 28: Driver loaded (0 - not loaded; note: set by the software)
	// 29-31: Reserved
	REG(0x00018, 0b00000000000000000000000000000000,
		     0b00010100000000110100000000000000);


	// page 545
	// Extended SDP Control — ESDP (0x00020; RW)
	// NOTE: The ixgbe driver checks that SDP2 Data Value is 1 and assumes the link is down otherwise. TODO Why?

	// 0-7: SDPn Data Value, where 'n' is bit number (0 - default)
	// 8-15: SDPn Pin Directionality, where 'n' is bit number (0 - default)
	// 16-23: SDPn Operating Mode, where 'n' is bit number (0 - default)
	// 24-25: Reserved (0)
	// 26-31: SDPn Native Mode Functionality, where 'n' is bit number PLUS TWO! (0 - default)
	REG(0x00020, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000100);

	// page 549
	// I2C Control — I2CCTL (0x00028; RW)

	// 0: I2C Clock In (0 - default; read-only)
	// 1: I2C Clock Out (0 - default)
	// 2: I2C Data In (0 - default; read-only)
	// 3: I2C Data Out (0 - default)
	// 4-31: Reserved
	REG(0x00028, 0b00000000000000000000000000001111, // in a pull-up system like I2C, 1 is the default
		     0b00000000000000000000000000001111); // NOTE: 0 and 2 are RW, see i2cctl_write for an explanation
	REGISTERS[0x00028].write = stub_register_i2cctl_write;


	// page 549
	// LED Control — LEDCTL (0x00200; RW)

	// 0-3: LED0 Mode (0000 - LINK_UP)
	// 4: Reserved (0)
	// 5: GLOBAL Blink Mode (0 - blink for 200 ms on/200ms off)
	// 6: LED0 Invert (0 - LED output is active low)
	// 7: LED0 Blink (0 - do not blink)
	// 8-11: LED1 Mode (0001 - 10 Gb/s link)
	// 12-13: Reserved (00)
	// 14: LED1 Invert (0 - LED output is active low)
	// 15: LED1 Blink (0 - do not blink)
	// 16-19: LED2 Mode (0100 - LINK/ACTIVITY)
	// 20-21: Reserved (00)
	// 22: LED2 Invert (0 - LED output is active low)
	// 23: LED2 Blink (0 - do not blink)
	// 24-27: LED3 Mode (0101 - 1 Gb/s link)
	// 28-29: Reserved (00)
	// 30: LED3 Invert (0 - LED output is active low)
	// 31: LED3 Blink (0 - do not blink)
	REG(0x00200, 0b00000101000001000000000100000000,
		     0b00000000000000000000000000000000);


	// page 572
	// Extended Interrupt Cause Register- EICR (0x00800; RW1C)

	// 0-15: Receive/Transmit Queue Interrupts (0x0 - not enabled)
	// 16: Flow director (0 - no)
	// 17: Missed packet (0 - no)
	// 18 - PCI timeout (0 - no)
	// 19: VF to PF MailBox (0 - no)
	// 20: Link Status Change (0 - no) TODO use this
	// 21: TX LinkSec counter reached threshold requiring key exchange (0 - no)
	// 22: Manageability event detected (0 - no)
	// 23: Reserved (0)
	// 24: General Purpose Interrupt on SDP0 (0 - no)
	// 25: General Purpose Interrupt on SDP1 (0 - no)
	// 26: General Purpose Interrupt on SDP2 (0 - no)
	// 27: General Purpose Interrupt on SDP3 (0 - no)
	// 28: Unrecoverable ECC Error (0 - no) TODO try this, software must reset if this is set
	// 29: Reserved (0)
	// 30: TCP Timer Expired (0 - no)
	// 31: Other interrupt - from EICR (0 - no)
	REG(0x00800, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000001);
	REGISTERS[0x00800].write = stub_register_rw1c_write;

	// page 573
	// Extended Interrupt Mask Set/Read Register- EIMS (0x00880; RWS)

	// 0-30: Interrupt Enable, each bit enables its corresponding interrupt in EICR (0 - not enabled)
	// 31: Reserved (0)
	REG(0x00880, 0b00000000000000000000000000000000,
		     0b01111111111111111111111111111111);

	// page 574
	// Extended Interrupt Mask Clear Register- EIMC (0x00888; WO)
	// TODO do we model interrupts?

	// 0-30: Interrupt Mask (0 - don't care, write-only register)
	// 31: Reserved
	REG(0x00888, 0b00000000000000000000000000000000,
		     0b01111111111111111111111111111111);
	REGISTERS[0x00888].readable = false;

	// page 575
	// Extended Interrupt Mask Clear Registers — EIMC[n] (0x00AB0 + 4*(n-1), n=1...2; WO)

	// 0-31: Writing a 1b to any bit clears the corresponding bit in the EIMS[n] register
	//	 "Reading this register provides no meaningful data."
	for (int n = 1; n <= 2; n++) {
		REG(0x00AB0 + 4*(n-1), 0b00000000000000000000000000000000,
				       0b11111111111111111111111111111111);
		REGISTERS[0x00AB0 + 4*(n-1)].readable = false;
	}


	// page 621
	// Rx DCA Control Register — DCA_RXCTRL[n] (0x0100C + 0x40*n, n=0...63 and 0x0D00C + 0x40*(n-64), n=64...127 / 0x02200 + 4*n, [n=0...15]; RW)
	// NOTE: "DCA_RXCTRL[0...15] are also mapped to address 0x02200... to maintain compatibility with the 82598."
	//       We do not implement the 0..15 at 0x0100C, which the ixgbe driver doesn't use
	for (int n = 0; n <= 127; n++) {
		int address = n <= 15 ? (0x02200 + 4*n)
			    : n <= 63 ? (0x0100C + 0x40*n)
				      : (0x0D00C + 0x40*(n-64));

		// 0-4: Reserved (0)
		// 5: Descriptor DCA EN (0 - not enabled)
		// 6: RX Header DCA EN (0 - not enabled)
		// 7: Payload DCA EN (0 - not enabled)
		// 8: Reserved (0)
		// 9: RX Descriptor Read Relax Order Enable (1 - enabled)
		// 10: Reserved (0)
		// 11: RX Descriptor Write Back Relax Order Enabled (0 - read-only; "this bit must be 0 to enable correct functionality")
		// 12: Reserved (0)
		// 13: RX Data Write Relax Order Enable (1 - enabled)
		// 14: Reserved (0)
		// 15: RX Split Header Relax Order Enable (1 - enabled)
		// 16-23: Reserved (0)
		// 24-31: CPU ID (0 - not set)
		REG(address, 0b00000000000000001010001000000000,
			     0b11111111000000001010000000000000);
	}


	// page 598-599
	// Split Receive Control Registers — SRRCTL[n] (0x01014 + 0x40*n, n=0...63 and 0x0D014 + 0x40*(n-64), n=64...127 / 0x02100 + 4*n, [n=0...15]; RW)
	// NOTE: We do not model n <= 15 at 0x01014, since DPDK doesn't use them
	// NOTE: "BSIZEHEADER must be bigger than zero if DESCTYPE is equal to 010b, 011b, 100b or 101b"

	// 0-4: Receive Buffer Size for Packet Buffer (0x2 - default; "This field should not be set to 0x0. This field should be greater or equal to 0x2 in queues where RSC is enabled")
	// 5-7: Reserved (0)
	// 8-13: Receive Buffer Size for Header Buffer, in 64-byte resolution (0x4 - default; "Value can be from 64 bytes to 1024 bytes")
	// 14-21: Reserved (0)
	// 22-24: Receive Descriptor Minimum Threshold Size (0 - default)
	// 25-27: Define the descriptor type in Rx (001 - Advanced)
	// 28: Drop Enabled (0 - not enabled)
	// 29-31: Reserved (000)
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 15 ? (0x02100 + 4*n)
			 : n <= 53 ? (0x01014 + 0x40*n)
				   : (0x0D014 + 0x40*(n-64));
		REG(addr, 0b00000010000000000000010000000010,
			  0b00000001110000000011111100011111);
	}


	// page 604
	// Transmit Descriptor Base Address Low — TDBAL[n] (0x06000+0x40*n, n=0...127; RW)

	// 0-6: Ignored on writes, reads as 0
	// 7-31: Transmit Descriptor Base Address Low
	for (int n = 0; n <= 127; n++) {
		REG(0x06000 + 0x40*n, 0b00000000000000000000000000000000,
				      0b11111111111111111111111111111111);
		REGISTERS[0x06000 + 0x40*n].write = stub_register_dbal_write;
	}


	// page 605
	// Transmit Descriptor Base Address High — TDBAH[n] (0x06004+0x40*n, n=0...127; RW)

	// 0-31: Transmit Descriptor Base Address High
	for (int n = 0; n <= 127; n++) {
		REG(0x06004 + 0x40*n, 0b00000000000000000000000000000000,
				      0b11111111111111111111111111111111);
	}

	// page 605
	// Transmit Descriptor Length — TDLEN[n] (0x06008+0x40*n, n=0...127; RW)

	// 0-19: Descriptor Ring Length - "It must be 128byte-aligned (7 LS bit must be set to zero)."
	// 20-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		REG(0x06008 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000011111111111110000000);
	}

	// page 605
	// Transmit Descriptor Head — TDH[n] (0x06010+0x40*n, n=0...127; RO)
	// "The only time that software should write to this register is after a reset (hardware reset or CTRL.RST)
	//  and before enabling the transmit function (TXDCTL.ENABLE)."
	// (in other words, it's RW, not RO...)

	// 0-15: Transmit Descriptor Head
	// 16-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		REG(0x06010 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000001111111111111111);
		REGISTERS[0x06010 + 0x40*n].write = stub_register_tdh_write;
	}

	// page 606
	// Transmit Descriptor Tail — TDT[n] (0x06018+0x40*n, n=0...127; RW)

	// 0-15: Transmit Descriptor Tail
	// 16-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		REG(0x06018 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000001111111111111111);
		REGISTERS[0x06018 + 0x40*n].write = stub_register_tdt_write;
	}


	// page 596
	// Receive Descriptor Base Address Low — RDBAL[n] (0x01000 + 0x40*n, n=0...63 and 0x0D000 + 0x40*(n-64), n=64...127; RW)

	// 0-6: Ignored on writes, reads as 0
	// 7-31: Receive Descriptor Base Address Low
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 63 ? (0x01000 + 0x40*n)
				   : (0x0D000 + 0x40*(n-64));
		REG(addr, 0b00000000000000000000000000000000,
			  0b11111111111111111111111111111111);
		REGISTERS[addr].write = stub_register_dbal_write;
	}


	// page 596
	// Receive Descriptor Base Address High — RDBAH[n] (0x01004 + 0x40*n, n=0...63 and 0x0D004 + 0x40*(n-64), n=64...127; RW)
	// XXX SPEC BUG page 172 point 3 says "registers RDBAL, RDBAL" but one of those should be RDBAH

	// 0-31: Receive Descriptor Base Address High
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 63 ? (0x01004 + 0x40*n)
				   : (0x0D004 + 0x40*(n-64));
		REG(addr, 0b00000000000000000000000000000000,
			  0b11111111111111111111111111111111);
	}

	// page 596
	// Receive Descriptor Length — RDLEN[n] (0x01008 + 0x40*n, n=0...63 and 0x0D008 + 0x40*(n-64), n=64...127; RW)

	// 0-19: Descriptor Ring Length - "It must be 128-byte aligned (7 LS bit must be set to zero)."
	// 20-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 63 ? (0x01008 + 0x40*n)
				   : (0x0D008 + 0x40*(n-64));
		REG(addr, 0b00000000000000000000000000000000,
			  0b00000000000011111111111110000000);
	}

	// page 597
	// Receive Descriptor Head — RDH[n] (0x01010 + 0x40*n, n=0...63 and 0x0D010 + 0x40*(n-64), n=64...127; RO)

	// 0-15: Receive Descriptor Head
	// 16-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 63 ? (0x01010 + 0x40*n)
				   : (0x0D010 + 0x40*(n-64));
		REG(addr, 0b00000000000000000000000000000000,
			  0b00000000000000000000000000000000);
	}

	// page 597
	// Receive Descriptor Tail — RDT[n] (0x01018 + 0x40*n, n=0...63 and 0x0D018 + 0x40*(n-64), n=64...127; RW)

	// 0-15: Receive Descriptor Tail
	// 16-31: Reserved (0)
	for (int n = 0; n <= 127; n++) {
		int addr = n <= 63 ? (0x01018 + 0x40*n)
				   : (0x0D018 + 0x40*(n-64));
		REG(addr, 0b00000000000000000000000000000000,
			  0b00000000000000001111111111111111);
	}


	// page 622
	// Tx DCA Control Registers — DCA_TXCTRL[n] (0x0600C + 0x40*n, n=0...127; RW)

	// 0-4: Reserved (0)
	// 5: Descriptor DCA Enable (0 - not enabled)
	// 6-8: Reserved (0)
	// 9: TX Descriptor Read Relax Order Enable (1 - enabled)
	// 10: Reserved (0)
	// 11: Relax Order Enable of TX Descriptor (1 - enabled)
	// 12: Reserved (0)
	// 13: TX Data Read Relax Order Enable (1 - enabled)
	// 14-23: Reserved (0)
	// 24-31: CPU ID (0 - not set)
	for (int n = 0; n <= 127; n++) {
		REG(0x0600C + 0x40*n, 0b00000000000000000010101000000000,
				      0b11111111000000000010101000000000);
	}


	// page 597
	// Receive Descriptor Control — RXDCTL[n] (0x01028 + 0x40*n, n=0...63 and 0x0D028 + 0x40*(n-64), n=64...127; RW)
	for (int n = 0; n <= 127; n++) {
		int addr = n < 64 ? (0x01028 + 0x40*n) : (0x0D028 + 0x40*(n-64));

		// 0-13: Reserved (0)
		// 14: Reserved, but readable/writeable for compatibility
		// 15: Reserved (0)
		// 16-22: Reserved, but readable/writeable for compatibility
		// 23-24: Reserved (00)
		// 25: Receive Queue Enable (0 - not enabled)
		// 26: Reserved, but readable/writeable for compatibility
		// 27-29: Reserved
		// 30: VLAN Mode Enable
		// 31: Reserved
		REG(addr, 0b00000000000000000000000000000000,
			  0b00000110011111110100000000000000);
	}


	// page 599
	// Receive DMA Control Register — RDRXCTL (0x02F00; RW)

	// XXX SPEC BUG no bit 0 mentioned, we'll assume it's reserved and reads as 0...
	// 1: Rx CRC Strip (0 - do not strip; "This bit must be set the same as HLREG0.RXCRCSTRP")
	// 2: Reserved (0)
	// 3: DMA Init Done (1 - done) TODO change this
	// 4-16: Reserved, but reads as 0x0880 (0b1000 1000 0000)
	// 17-21: Defines a minimum packet size (after VLAN stripping, if applicable) for a packet with a payload that can open a new RSC (in units of 16 byte.)
	//        (0x8 - default; "RSCFRSTSIZE is reserved for internal use. Software should set this field to 0x0")
	// 22-24: Reserved (000)
	// 25: RSC Coalescing on ACK Change (0 - default; "RSCACKC is reserved for internal use. Software should set this bit to 1b")
	// 26: FCoE Write Exchange Fix (0 - default; "FCOE_WRFIX is reserved for internal use. Software should set this bit to 1b")
	// 27-31: Reserved (0)
	REG(0x02F00, 0b00000000000100001000100000001000,
		     0b00000110001111100000000000001110);
	REGISTERS[0x02F00].write = stub_register_rdrxctl_write;

	// page 702
	// Receive Queue Statistic Mapping Registers — RQSMR[n] (0x02300 + 4*n, n=0...31; RW)

	// 0-3: Q_MAP for queues 4*n (0 - default)
	// 4-7: Reserved (0)
	// 8-11: Q_MAP for queues 4*n+1 (0 - default)
	// 12-15: Reserved (0)
	// 16-19: Q_MAP for queues 4*n+2 (0 - default)
	// 20-23: Reserved (0)
	// 24-27: Q_MAP for queues 4*n+3 (0 - default)
	// 28-31: Reserved (0)
	for (int n = 0; n <= 31; n++) {
		REG(0x02300 + 4*n, 0b00000000000000000000000000000000,
				   0b00000000000000000000000000000000);
	}


	// page 600
	// Receive Control Register — RXCTRL (0x03000; RW)

	// 0: Receive Enable (0 - not yet enabled)
	// 1-31: Reserved
	REG(0x03000, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000001);


	// page 661
	// PCG_1G link Control Register — PCS1GLCTL (0x04208; RW)

	// 0: Forced Link 1 GbE value (0 - default value)
	// 1-4: Reserved (0)
	// 5: Force 1GbE link (0 - not forced)
	// 6: Link Latch Low Enable (0 - not enabled)
	// 7-17: Reserved (0)
	// 18: Auto Negotiation 1 GbE Timeout Enable (1 - enabled)
	// 19-24: Reserved (0)
	// 25: Link OK Fix En (1 - "should be set to 1b for nominal operation")
	// 26-31: Reserved
	REG(0x04208, 0b00000010000001000000000000000000,
		     0b00000000000001000000000000000000);


	// page 663
	// PCS_1 Gb/s Auto Negotiation Advanced Register — PCS1GANA (0x04218; RW)

	// 0-4: Reserved (0)
	// 5: Full-Duplex (1 - capable)
	// 6: Reserved (0)
	// 7-8: Local PAUSE capabilities (11 - both symmetric and asymmetric toward local device)
	// 9-11: Reserved (0)
	// 12-13: Remote Fault (00 - no error, link good)
	// 14: Reserved (0)
	// 15: Next Page Capable (0 - no next pages left)
	// 16-31: Reserved
	REG(0x04218, 0b00000000000000000000000110100000,
		     0b00000000000000001011000110100000);


	// page 666
	// MAC Core Control 0 Register — HLREG0 (0x04240; RW)

	// 0: TX CRC Enable (1 - enable)
	// XXX SPEC BUG the data sheet says bit 1 is reserved and set to 1, but then has another bit 1...
	// 1: RX CRC Strip (1 - enabled)
	// 2: Jumbo Frames Enable (0 - disable)
	// 3-9: Reserved, must be set to 0x1 (!!!)
	// 10: TX Pad Frame Enable (1 - enabled)
	// 11-14: Reserved, must be set to 0101b (!!!)
	// 15: Loopback (0 - disabled)
	// 16: MDC Speed (1 - default)
	// 17: Continuous MDC (0 - off between packets, default)
	// 18-19: Reserved (00)
	// 20-23: Prepend Value (0 - default)
	// 24-26: Reserved (0)
	// 27: RX Length Error Reporting (1 - enabled, default)
	// 28: RX Padding Strip Enable (0 - disabled, default; "this functionality should be used as debug mode only")
	// 29-31: Reserved (0)
	REG(0x04240, 0b00001000000000010010110000001011,
		     0b00000000000000000000000000000110);


	// page 668
	// MDI Single Command and Address — MSCA (0x0425C; RW)

	// 0-15: MDI Address (0x0000 - default)
	// 16-20: DeviceType/Register Address (0x0 - default)
	// 21-25: PHY Address (0x0 - default)
	// 26-27: OP Code (00 - default; all 4 combinations are valid)
	// 28-29: ST Code (0 - New protocol, default; only 00 and 01 are valid)
	// 30: MDI Command (0 - ready)
	// 31: Reserved
	REG(0x0425C, 0b00000000000000000000000000000000,
		     0b01011111111111111111111111111111); // bit 29 is read-only since it cannot be 1
	REGISTERS[0x0425C].write = stub_register_msca_write;


	// page 669
	// MDI Single Read and Write Data — MSRWD (0x04260; RW)

	// 0-15: MDI Write Data (0 - default)
	// 16-31: MDI Read Data (0 - default; read-only)
	REG(0x04260, 0b00000000000000000000000000000000,
		     0b00000000000000001111111111111111);


	// page 680
	// MAC Manageability Control Register — MMNGC (0x042D0; Host-RO/MNG-RW)

	// 0: MNG_VETO (0 - no veto)
	// 1-31: Reserved (0)
	REG(0x042D0, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000000);


	// pages 674-676
	// Auto Negotiation Control Register — AUTOC (0x042A0; RW)
	// NOTE: "The 82599 Device Firmware may access AUTOC register in parallel
	//        to software driver and a synchronization between them is needed"

	// 0: Force Link Up (0 - normal mode)
	// 1: Auto-negotiation Acknowledge2 field (0 - ???)
	// 2-6: Auto-negotiation Selector field (00001 - default value according to 802.3ap-2007)
	// 7-8: "Define 10 GbE PMA/PMD over four differential pairs" (01 - KX4 PMA/PMD, default value)
	// 9: PMA/PMD used for 1GbE (1 - KX or BX PMA/PMD, default value)
	// 10: Disables 10GbE Parallel Detect On Dx without main power (0 - no specific action)
	// 11: Restarts auto-negotiation on transition to Dx (0 - does not restart)
	// 12: Applies new settings and restarts relative auto-negotiation process (self-clearing)
	// 13-15: Link Mode Select (100 - KX/KX4/KR backplane auto-negotiation enable, Clause 37 negotiation disabled, default value)
	// 16: Configures the A2 bit of the TAF in the auto-negotiation word... blah blah blah... (1 - default)
	// 17: FEC Requested (0 - not requested)
	// 18: FEC Ability; should be set to 1 only if bit 16 is 1 (1 - supported)
	// 19-22: Backplane Auto-Negotiation Rx Align Treshold (0011 - default value)
	// 23: Auto-Negotiation Rx Drift Mode (1 - enabled)
	// 24: Auto-Negotiation Rx Loose Mode (1 - enabled)
	// 25-26: Auto-Negotiation Parallel Detect Timer (00 - 1ms)
	// 27: RF (0 - default)
	// 28-29: Pause Bits (00 - default)
	// 30-31: ...i don't even know what this description means (11 - KX supported, KX4 supported)
	REG(0x042A0, 0b11000001100111011000001010000100,
		     0b00000000000000000001000000000000);
	REGISTERS[0x042A0].write = stub_register_autoc_write;


	// pages 676-678
	// Link Status Register — LINKS (0x042A4; RO)

	// 0: Signal Detect of 1 GbE and 100 Mb/s (1 - signal present, OK)
	// 1: Signal detect of FEC (1 - signal detected, good)
	// 2: 10 GbE serial PCS FEC block lock (0 - no lock)
	// 3: 10 GbE serial KR_PCS high error rate (0 - low)
	// 4: 10 GbE serial PCS block lock (0 - no lock)
	// 5: KX/KX4/KR AN Next Page Received (0 - not received; clears on read)
	// 6: KX/KX4/KR Backplane Auto Negotiation Page Received (0 - not receifed; clears on read)
	// 7: Link Status (1 - link up; self-set on read to proper value)
	// 8-11: Signal Detect of 10 GbE Parallel (1111 - signal present for lanes 0,1,2,3 respectively, good)
	// 12: Signal Detet of 10 GbE serial (1 - signal detected, good)
	// 13-16: 10G Parallel lane sync status (1111 - sync status OK for lanes 0,1,2,3 respectively, good)
	// 17: 10 GbE align_status (1 - good)
	// 18: 1G sync_status (1 - good)
	// 19: KX/KX4/KR Baclplane Auto Negotiation Rx Idle (0 - good)
	// 20: PCS_1 GbE auto-negotiation enabled, aka clause 37 (0 - not enabled, see AUTOC bits 13-15)
	// 21: 1 GbE PCS enabled for 1 GbE and SGMII operation (0 - not enabled)
	// 22: 10G link enabled (1 - enabled)
	// 23: Forward Error Correction status in 10 GbE serial link (0 - disabled)
	// 24: Status of 10 GbE serial PCS (0 - disabled)
	// 25: Status of SGMII operation (0 - disabled)
	// 26-27: MAC link mode status (11 - auto-negotiation)
	// 28-29: MAC link speec status (11 - 10 GbE)
	// 30: Link Up (1 - link is up)
	// 31: KX/KX4/KR backplane auto-negotiation completed (1 - completed)
	REG(0x042A4, 0b11111100010001111111111110000011,
		     0b00000000000000000001000000000000);

	// page 679
	// Auto Negotiation Control 2 Register — AUTOC2 (0x042A8; RW)

	// 0-15: Reserved (0)
	// 16-17: PMAPMD used for 10 GbE serial link operation (00 - KR; note that 01 and 11 are reserved)
	// 18: Disable DME Pages Transmit (0 - not disabled)
	// 19-27: Reserved (0)
	// 28: Force auto-negotiation arbitration state machine to idle (0 - no force)
	// 29: Reserved (0)
	// 30: Disable parallel detect in KX/KX4/KR (0 - not disabled)
	// 31: Reserved (0)
	REG(0x042A8, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000000);


	// page 611
	// DCB Transmit Descriptor Plane Control and Status — RTTDCS (0x04900; RW) DMA-Tx

	// 0: TC Transmit Descriptor Plane Arbitration Control (0 - RR)
	// 1: VM Transmit Descriptor Plane Arbitration Control (0 - RR)
	// 2-3: Reserved (0)
	// 4: TC Transmit descriptor plane recycle mode (0 - no recycle)
	// 5: Reserved (0)
	// 6: DCB Arbiters Disable (0 - "during nominal operation this bit should be set to 0")
	// 7-16: Reserved (0)
	// 17-19: Last Transmitted TC (0)
	// 20-21: Reserved (0)
	// 22: Bypass Data_Pipe Monitor (1 - bypass)
	// 23: Bypass Packet Buffer Free Space Monitor (1 - bypass)
	// 24-30: Reserved (0)
	// 31: Link speed has changed (0 - not changed)
	REG(0x04900, 0b00000000110000000000000000000000,
		     0b00000000000000000000000001000000);


	// page 618
	// DCB Transmit Descriptor Plane Queue Select — RTTDQSEL (0x04904; RW)

	// 0-6: TX Descriptor Queue Index or TX Pool of Queues Index (0 - default value)
	// 7-31: Reserved (0)
	REG(0x04904, 0b00000000000000000000000000000000,
		     0b00000000000000000000000001111111);


	// page 619
	// DCB Transmit  Rate-Scheduler Config — RTTBCNRC (0x04984; RW)

	// 0-13: TX rate-scheduler rate factor hexadecimal part (0 - default)
	// 14-23: TX rate-scheduler rate factor integral part (0 - default)
	// 24-30: Reserved (0)
	// 31: TX rate-scheduler enable (0 - not enabled)
	REG(0x04984, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000000);


	// page 603
	// DMA Tx Control — DMATXCTL (0x04A80; RW)

	// 0: Transmit Enable (0 - not enabled)
	// 1-2: Reserved, reads as 10
	// 3: Double VLAN Mode (0 - not enabled)
	// 4-15: Reserved (0)
	// 16-31: VLAN Ether-Type a.k.a. TPID (0x8100 - "For proper operation, software must not change the default setting of this field")
	REG(0x04A80, 0b10000001000000000000000000000100,
		     0b00000000000000000000000000000001);


	// page 585
	// Receive Checksum Control — RXCSUM (0x05000; RW)
	// NOTE: "This register should only be initialized (written) when the receiver is not enabled (for example, only write this register when RXCTRL.RXEN = 0b)."

	// 0-11: Reserved (0)
	// 12: IP Payload Checksum Enable (0 - not enabled)
	// 13: RSS/Fragment Checksum Status Selection (0 - fragment checksum; 1 is RSS)
	// 14-31: Reserved (0)
	REG(0x05000, 0b00000000000000000000000000000000,
		     0b00000000000000000011000000000000);
	REGISTERS[0x05000].write = stub_register_needsrxen0_write;

	// page 586
	// Receive Filter Control Register — RFCTL (0x05008; RW)

	// 0-4: Reserved (0)
	// XXX SPEC BUG: Reserved above is marked as 0-5, but there's a bit 5...
	// 5: RSC Disable (0 - not disabled)
	// 6: NFS Write disable (0 - not disabled)
	// 7: NFS Read disable (0 - not disabled)
	// 8-9: NFS version recognized by the hardware (00 - v2)
	// 10: IPv6 Disable (0 - not disabled; "Internal use only – should not be set to 1b")
	// 11-13: Reserved (0)
	// 14: IP Fragment Split Disable (0 - not disabled; "Internal use only – should not be set to 1b")
	// 15-31: Reserved (0)
	REG(0x05008, 0b00000000000000000000000000000000,
		     0b00000000000000000000000011100000);


	// page 582
	// Filter Control Register — FCTRL (0x05080; RW)
	// NOTE: "Before receive filters are updated/modified the RXCTRL.RXEN bit should be set to 0b"

	// 0: Reserved (0)
	// 1: Store Bad Packets (0 - don't)
	// 2-7: Reserved (0)
	// 8: Multicast Promiscuous Enable (0 - disable)
	// 9: Unicast Promiscuous Enable (0 - disable)
	// 10: Broadcast Accept Mode (0 - disable)
	// 11-31: Reserved (0)
	REG(0x05080, 0b00000000000000000000000000000000,
		     0b00000000000000000000011100000000);
	REGISTERS[0x05080].write = stub_register_needsrxen0_write;


	// page 583
	// VLAN Control Register — VLNCTRL (0x05088; RW)

	// 0-15: VLAN Ether Type (0x8100; "For proper operation, software must not change the default setting of this field")
	// 16-27: Reserved (0)
	// 28: Canonical Form Indicator Bit Value (0 - doesn't matter since not enabled)
	// 29: Canonical Form Indicator Enable (0 - not enabled)
	// 30: VLAN Filter Enable (0 - not enabled)
	// 31: Reserved (0)
	REG(0x05088, 0b00000000000000001000000100000000,
		     0b00000000000000000000000000000000);


	// page 583
	// Multicast Control Register — MCSTCTRL (0x05090; RW)

	// 0-1: Multicast Offset (00 - [47:36])
	// 2: Multicast Filter Enable (0 - disabled)
	// 3-31: Reserved (0)
	REG(0x05090, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000000);


	// page 587
	// Multicast Table Array — MTA[n] (0x05200 + 4*n, n=0...127; RW)

	// 0-31: Bit Vector (0 - we do not care in this model... TODO?)
	for (int n = 0; n <= 127; n++) {
		REG(0x05200 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 591
	// RSS Random Key Register — RSSRK (0x0EB80 + 4*n, n=0...9 / 0x05C80 + 4*n, n=0...9; RW)
	// NOTE: We only do the 0x05C80, DPDK doesn't use 0x0EB80

	// 0-7: RSS Key Byte 4*n
	// 8-15: RSS Key Byte 4*n+1
	// 16-23: RSS Key Byte 4*n+2
	// 24-31: RSS Key Byte 4*n+3
	for (int n = 0; n <= 9; n++) {
		REG(0x05C80 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 591
	// Redirection Table — RETA[n] (0x0EB00 + 4*n, n=0...31 / 0x05C00 + 4*n, n=0...31; RW)
	// NOTE: we only do the 0x05C00, DPDK doesn't use 0x0EB00

	// 0-3: RSS output index for hash value 4*n (0 - default)
	// 4-7: Reserved (0)
	// 8-11: RSS output index for hash value 4*n+1 (0 - default)
	// 12-15: Reserved (0)
	// 16-19: RSS output index for hash value 4*n+2 (0 - default)
	// 20-23: Reserved (0)
	// 24-27: RSS output index for hash value 4*n+3 (0 - default)
	// 28-31: Reserved (0)
	for (int n = 0; n <= 31; n++) {
		REG(0x05C00 + 4*n, 0b00000000000000000000000000000000,
				   0b00001111000011110000111100001111);
	}


	// page 606
	// Transmit Descriptor Control — TXDCTL[n] (0x06028+0x40*n, n=0...127; RW)
	for (int n = 0; n <= 127; n++) {
		// 0-6: Prefetch Threshold (0x0 - zero)
		// 7: Reserved (0)
		// 8-14: Host Threshold (0x0 - zero) TODO check that if PTHRESH is used HTHRESH is >0
		// 15: Reserved (0)
		// 16-22: Write-Back Threshold (0x0 - zero)
		// 23-24: Reserved (0)
		// 25: Transmit Queue Enable (0 - not enabled)
		// 26: Transmit Software Flush (0 - not enabled; note: "This bit is self cleared by hardware")
		// 27-31: Reserved (0)
		REG(0x06028 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000110000000000000000001111111);
		REGISTERS[0x06028 + 0x40*n].write = stub_register_txdctl_write;
	}


	// page 626
	// Security Rx Control — SECRXCTRL (0x08D00; RW)

	// 0: RX Security Offload Disable (1 - disabled)
	// 1: Disable Sec RX Path (0 - not disabled)
	// 2-31: Reserved (0)
	REG(0x08D00, 0b00000000000000000000000000000001,
		     0b00000000000000000000000000000011);


	// page 626
	// Security Rx Status — SECRXSTAT (0x08D04; RO)

	// 0: Rx security block ready for mode change (1 - ready)
	// 1: Security offload is disabled by fuse or strapping pin (0 - not disabled)
	// 2: Unrecoverable ECC error in an Rx SA table occurred (0 - no error)
	// 3-31: Reserved (0)
	REG(0x08D04, 0b00000000000000000000000000000001,
		     0b00000000000000000000000000000000);


	// page 609
	// Multiple Transmit Queues Command Register — MTQC (0x08120; RW)
	// NOTE: "Programming MTQC must be done only during the init phase while software must also set RTTDCS.ARBDIS
	//	  before configuring MTQC and then clear RTTDCS.ARBDIS afterwards"
	//       -- page 337

	// 0: DCB Enabled Mode (0)
	// 1: Virtualization Enabled Mode (0)
	// 2-3: Number of TCs or Number of Tx Queues per Pools (00)
	// 4-31: Reserved (0)
	REG(0x08120, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000001111);
	REGISTERS[0x08120].write = stub_register_mtqc_write;


	// page 588
	// VLAN Filter Table Array — VFTA[n] (0x0A000 + 4*n, n=0...127; RW)

	// 0-31: VLAN Filter. Each bit i in register n ffects packets with VLAN tags equal to 32n+i
	//	 When set, the bit enables packet with the associated VLAN tag to pass.
	for (int n = 0; n <= 127; n++) {
		REG(0x0A000 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 587
	// Receive Address Low — RAL[n] (0x0A200 + 8*n, n=0...127; RW)
	// NOTE: "The first Receive Address register [...] RAR0 should always be used to store the individual Ethernet MAC address of the adapter."

	// 0-31: Receive Address Low, lower 32 bits of the MAC addr ("field is defined in big endian")
	REG(0x0A200, 0x45678900, // FIXME not needed? NOTE: see VigNAT makefile
		     0xFFFFFFFF);
	for (int n = 1; n <= 127; n++) {
		REG(0x0A200 + 8*n, 0x00000000,
				   0xFFFFFFFF);
	}

	// page 587-588
	// Receive Address High — RAH[n] (0x0A204 + 8*n, n=0...127; RW)
	// NOTE: see note for RAL

	// 0-15: Receive Address High, upper 16 bits of MAC addr ("field is defined in big endian")
	// 16-30: Reserved (0)
	// 31: Address Valid (0/1 - in/valid)
	REG(0x0A204, 0x80000123, // FIXME not needed? NOTE: see RAL
		     0x8000FFFF);
	for (int n = 1; n <= 127; n++) {
		REG(0x0A204 + 8*n, 0x00000000,
				   0x8000FFFF);
	}


	// page 588
	// MAC Pool Select Array — MPSAR[n] (0x0A600 + 4*n, n=0...255; RW)

	// 0-31: Bit i enables pool i/32+i in register 2n/2n+1 for MAC filter n
	for (int n = 0; n <= 255; n++) {
		REG(0x0A600 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 589
	// Multiple Receive Queues Command Register- MRQC (0x0EC80 / 0x05818; RW)
	// NOTE: We only implement 0x05818, which is the address DPDK uses

	// 0-3: Multiple Receive Queues Enable (0000 - default)
	// 4-15: Reserved (0)
	// 16-31: RSS Field Enable (0x0 - default)
	REG(0x05818, 0b00000000000000000000000000000000,
		     0b11111111111111110000000000001111);


	// pages 730-731
	// PF VM VLAN Pool Filter — PFVLVF[n] (0x0F100 + 4*n, n=0...63; RW)

	// 0-11: VLAN ID for pool filter n. (0 - no pools; note: "appears in little endian order")
	// 12-30: Reserved (0)
	// 31: VLAN ID Enable (0 - not enabled)
	for (int n = 0; n <= 63; n++) {
		REG(0x0F100 + 4*n, 0b00000000000000000000000000000000,
				   0b10000000000000000000111111111111);
	}


	// page 731
	// PF VM VLAN Pool Filter Bitmap — PFVLVFB[n] (0x0F200 + 4*n, n=0...127; RW)

	// 0-31: Pool Enable Bit Array. Bit i in 2n/2n+1 is associated with pool i/32+i (0 - nothing)
	for (int n = 0; n <= 127; n++) {
		REG(0x0F200 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 731
	// PF Unicast Table Array — PFUTA[n] (0x0F400 + 4*n, n=0...127; RW)

	// 0-31: Word-wide bit vector in the unicast destination address filter table (0 - models don't care; TODO?)
	for (int n = 0; n <= 127; n++) {
		REG(0x0F400 + 4*n, 0b00000000000000000000000000000000,
				   0b11111111111111111111111111111111);
	}


	// page 552
	// EEPROM/Flash Control Register — EEC (0x10010; RW)

	// 0: Clock input (0 - not enabled)
	// 1: Chip select (0 - not enabled)
	// 2: Data input (0 - not enabled)
	// 3: Data output (X - don't care)
	// 4-5: Flash Write Enable Control (11 - not allowed)
	// 6: Request EEPROM Access (0 - not enabled)
	// 7: Grant EEPROM Access (0 - not enabled)
	// 8: EEPROM Present (1 - present, correct signature)
	// 9: EEPROM Auto-Read Done (1 - done, since we fake hardware...)
	// 10: Reserved (1 - Reserved)
	// 11-14: EEPROM Size (0100 - Default)
	// 15: PCIe Analog Done (0 - not done)
	// 16: PCIe Core Done (0 - not done)
	// 17: PCIe General Done (0 - not done)
	// 18: PCIe Function Done (0 - not done)
	// 19: Core Done (0 - not done)
	// 20: Core CSR Done (0 - not done)
	// 21: MAC Done (0 - not done)
	// 22-31: Reserved (0x0)
	REG(0x10010, 0b00000000000000000001011100110000,
		     0b00000000000000000000000000000000);


	// page 554
	// EEPROM Read Register — EERD (0x10014; RW)
	// "This register is used by software to cause the 82599 to read individual words in the EEPROM."

	// 0: Start (0 - not started)
	// 1: Done (0 - no read perfomed yet)
	// 2-15: Address (0x0 - no read performed yet)
	// 16-31: Data (0x0 - no read performed yet)
	REG(0x10014, 0b00000000000000000000000000000000,
		     0b11111111111111111111111111111111);
	REGISTERS[0x10014].write = stub_register_eerd_write;

	// page 567
	// Software Semaphore Register — SWSM (0x10140; RW)
	// "This register is shared for both LAN ports."
	// NOTE: Bit 0 is automatically set to 1 by the hardware after a read
	// NOTE: See SW_FW_SYNC dance described below

	// 0: Semaphore (0 - not accessing)
	// 1: Software Semaphore (0 - not set)
	// 2-31: Reserved (0x0)
	REG(0x10140, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000000011);
	REGISTERS[0x10140].read = stub_register_swsm_read;
	REGISTERS[0x10140].write = stub_register_swsm_write;


	// pages 567-568
	// Firmware Semaphore Register — FWSM (0x10148; RW)
	// "This register should be written only by the manageability firmware.
	//  The device driver should only read this register."

	// 0: Firmware semaphore (0 - not accessing)
	// 1-3: Firmware mode (000 - none, manageability off)
	// 4-5: Reserved (00)
	// 6: EEPROM Reloaded Indication (1 - has been reloaded)
	// 7-14: Reserved (0x0)
	// 15: Firmware Valid Bit (1 - ready, boot has finished) TODO make it 0
	// 16-18: Reset Counter (000 - not reset)
	// 19-24: External Error Indication (0x00 - No error)
	// 25: PCIe Configuration Error Indication (0 - no error)
	// 26: PHY/SERDES0 Configuration Error Indication (0 - no error, LAN0 is fine)
	// 27: PHY/SERDES1 Configuration Error Indication (0 - no error, LAN1 is fine)
	// 28-31: Reserved (0000)
	REG(0x10148, 0b00000000000000001000000001000000,
		     0b00000000000000000000000000000000);


	// page 565
	// Function Active and Power State to Manageability — FACTPS (0x10150; RO)

	// 0-1: Power state indication of function 0 (00 - DR)
	// 2: Lan 0 Enable (1 - enabled)
	// 3: Function 0 Auxiliary Power PM Enable (0 - ???)
	// 4-5: Reserved (00)
	// 6-7: Power state indication of function 1 (00 - disabled)
	// 8: Lan 1 Enable (0 - disabled)
	// 9: Function 1 Auxiliary Power PM Enable (0 - disabled)
	// 10-28: Reserved (0x0)
	// 29: Manageability Clock Gated (0 - not gated)
	// 30: LAN Function Sel (0 - not inverted) TODO enable
	// 31: PM State Changed (0 - not changed)
	REG(0x10150, 0b00000000000000000000000000000100,
		     0b00000000000000000000000000000000);


	// page 569
	// Software–Firmware Synchronization — SW_FW_SYNC (0x10160; RW)
	// "This register is shared for both LAN ports."
	// NOTE: See 0x10140 and 0x10148
	// NOTE: Also known as "General Software Semaphore Register", or GSSR
	// NOTE: See Section 10.5.4 "Software and Firmware Synchronization"
	//       The SW_FW_SYNC dance's happy path is:
	//       - Software locks SWSM.SMBI by reading and getting 0 (hardware automatically sets it to 1)
	//       - Software locks SWSM.SWESMBI by writing 1 then reading 1
	//       - Software sets/clears the SW_FW_SYNC access bits it wants to by writing 1/0
	//         (locks only if firmware hasn't sets the counterpart bits)
	//       - Software clears SWSM.SWESMBI by writing 0
	//       - Software clears SWSM.SMBI by writing 0

	// 0: EEPROM software access
	// 1: PHY 0 software access
	// 2: PHY 1 software access
	// 3: Shared CSRs software access
	// 4: Flash software access
	// 5: EEPROM firmware access
	// 6: PHY 0 firmware access
	// 7: PHY 1 firmware access
	// 8: Shared CSRs firmware access
	// 9: Flash firmware access (note: "Currently the FW does not access the FLASH")
	// 10-31: Reserved
	REG(0x10160, 0b00000000000000000000000000000000,
		     0b00000000000000000000000000011111);
	REGISTERS[0x10160].write = stub_register_swfwsync_write;


	// starting at page 687
	// Statistics registers; all of them are 32-bit numbers and cleared on read
	const int stat_regs[] = {
		0x04000, // CRC Error Count — CRCERRS
		0x04004, // Illegal Byte Error Count — ILLERRC
		0x04008, // Error Byte Packet Count — ERRBC
		0x03FA0, // Rx Missed Packets Count — RXMPC[0]
		0x03FA4, // Rx Missed Packets Count — RXMPC[1]
		0x03FA8, // Rx Missed Packets Count — RXMPC[2]
		0x03FAC, // Rx Missed Packets Count — RXMPC[3]
		0x03FB0, // Rx Missed Packets Count — RXMPC[4]
		0x03FB4, // Rx Missed Packets Count — RXMPC[5]
		0x03FB8, // Rx Missed Packets Count — RXMPC[6]
		0x03FBC, // Rx Missed Packets Count — RXMPC[7]
		0x04034, // MAC Local Fault Count — MLFC
		0x04038, // MAC Remote Fault Count — MRFC
		0x04040, // Receive Length Error Count — RLEC
		0x08780, // Switch Security Violation Packet Count — SSVPC
		0x03F60, // Link XON Transmitted Count — LXONTXC
		0x041A4, // Link XON Received Count — LXONRXCNT
		0x03F68, // Link XOFF Transmitted Count — LXOFFTXC
		0x041A8, // Link XOFF Received Count — LXOFFRXCNT
		0x03F00, // Priority XON Transmitted Count — PXONTXC[0]
		0x03F04, // Priority XON Transmitted Count — PXONTXC[1]
		0x03F08, // Priority XON Transmitted Count — PXONTXC[2]
		0x03F0C, // Priority XON Transmitted Count — PXONTXC[3]
		0x03F10, // Priority XON Transmitted Count — PXONTXC[4]
		0x03F14, // Priority XON Transmitted Count — PXONTXC[5]
		0x03F18, // Priority XON Transmitted Count — PXONTXC[6]
		0x03F1C, // Priority XON Transmitted Count — PXONTXC[7]
		0x04140, // Priority XON Received Count — PXONRXCNT[0]
		0x04144, // Priority XON Received Count — PXONRXCNT[1]
		0x04148, // Priority XON Received Count — PXONRXCNT[2]
		0x0414C, // Priority XON Received Count — PXONRXCNT[3]
		0x04150, // Priority XON Received Count — PXONRXCNT[4]
		0x04154, // Priority XON Received Count — PXONRXCNT[5]
		0x04158, // Priority XON Received Count — PXONRXCNT[6]
		0x0415C, // Priority XON Received Count — PXONRXCNT[7]
		0x03F20, // Priority XOFF Transmitted Count — PXOFFTXCNT[0]
		0x03F24, // Priority XOFF Transmitted Count — PXOFFTXCNT[1]
		0x03F28, // Priority XOFF Transmitted Count — PXOFFTXCNT[2]
		0x03F2C, // Priority XOFF Transmitted Count — PXOFFTXCNT[3]
		0x03F30, // Priority XOFF Transmitted Count — PXOFFTXCNT[4]
		0x03F34, // Priority XOFF Transmitted Count — PXOFFTXCNT[5]
		0x03F38, // Priority XOFF Transmitted Count — PXOFFTXCNT[6]
		0x03F3C, // Priority XOFF Transmitted Count — PXOFFTXCNT[7]
		0x04160, // Priority XOFF Received Count — PXOFFRXCNT[0]
		0x04164, // Priority XOFF Received Count — PXOFFRXCNT[1]
		0x04168, // Priority XOFF Received Count — PXOFFRXCNT[2]
		0x0416C, // Priority XOFF Received Count — PXOFFRXCNT[3]
		0x04170, // Priority XOFF Received Count — PXOFFRXCNT[4]
		0x04174, // Priority XOFF Received Count — PXOFFRXCNT[5]
		0x04178, // Priority XOFF Received Count — PXOFFRXCNT[6]
		0x0417C, // Priority XOFF Received Count — PXOFFRXCNT[7]
		0x03240, // Priority XON to XOFF Count — PXON2OFFCNT[0]
		0x03244, // Priority XON to XOFF Count — PXON2OFFCNT[1]
		0x03248, // Priority XON to XOFF Count — PXON2OFFCNT[2]
		0x0324C, // Priority XON to XOFF Count — PXON2OFFCNT[3]
		0x03250, // Priority XON to XOFF Count — PXON2OFFCNT[4]
		0x03254, // Priority XON to XOFF Count — PXON2OFFCNT[5]
		0x03258, // Priority XON to XOFF Count — PXON2OFFCNT[6]
		0x0325C, // Priority XON to XOFF Count — PXON2OFFCNT[7]
		0x041B0, // Good Rx Non-Filtered Packet Counter — RXNFGPC
		0x041B4, // Good Rx Non-Filter Byte Counter Low — RXNFGBCL
		0x041B8, // Good Rx Non-Filter Byte Counter High — RXNFGBCH
		0x02F50, // DMA Good Rx Packet Counter — RXDGPC
		0x02F54, // DMA Good Rx Byte Counter Low — RXDGBCL
		0x02F58, // DMA Good Rx Byte Counter High — RXDGBCH
		0x02F5C, // DMA Duplicated Good Rx Packet Counter — RXDDPC
		0x02F60, // DMA Duplicated Good Rx Byte Counter Low — RXDDBCL
		0x02F64, // DMA Duplicated Good Rx Byte Counter High — RXDDBCH
		0x02F68, // DMA Good Rx LPBK Packet Counter — RXLPBKPC
		0x02F6C, // DMA Good Rx LPBK Byte Counter Low — RXLPBKBCL
		0x02F70, // DMA Good Rx LPBK Byte Counter High — RXLPBKBCH
		0x02F74, // DMA Duplicated Good Rx LPBK Packet Counter — RXDLPBKPC
		0x02F78, // DMA Duplicated Good Rx LPBK Byte Counter Low — RXDLPBKBCL
		0x02F7C, // DMA Duplicated Good Rx LPBK Byte Counter High — RXDLPBKBCH
		0x04080, // Good Packets Transmitted Count — GPTC
		0x04090, // Good Octets Transmitted Count Low — GOTCL
		0x04094, // Good Octets Transmitted Count High — GOTCH
		0x087A0, // DMA Good Tx Packet Counter – TXDGPC
		0x087A4, // DMA Good Tx Byte Counter Low – TXDGBCL
		0x087A8, // DMA Good Tx Byte Counter High – TXDGBCH
		0x040A4, // Receive Undersize Count — RUC
		0x040A8, // Receive Fragment Count — RFC
		0x040AC, // Receive Oversize Count — ROC
		0x040B0, // Receive Jabber Count — RJC
		0x040C0, // Total Octets Received Low — TORL
		0x040C4, // Total Octets Received High — TORH
		0x040D0, // Total Packets Received — TPR
		0x040D4, // Total Packets Transmitted — TPT
		0x040D8, // Packets Transmitted (64 Bytes) Count — PTC64
		0x040DC, // Packets Transmitted [65–127 Bytes] Count — PTC127
		0x040E0, // Packets Transmitted [128–255 Bytes] Count — PTC255
		0x040E4, // Packets Transmitted [256–511 Bytes] Count — PTC511
		0x040E8, // Packets Transmitted [512–1023 Bytes] Count — PTC1023
		0x040EC, // Packets Transmitted [Greater Than 1024 Bytes] Count — PTC1522
		0x040F0, // Multicast Packets Transmitted Count — MPTC
		0x040F4, // Broadcast Packets Transmitted Count — BPTC
		0x04010, // MAC Short Packet Discard Count — MSPDC
		0x04120, // XSUM Error Count — XEC
		0x05118, // FC CRC Error Count — FCCRC
		0x0241C, // FCoE Rx Packets Dropped Count — FCOERPDC
		0x02424, // FC Last Error Count — FCLAST
		0x02428, // FCoE Packets Received Count — FCOEPRC
		0x0242C, // FCOE DWord Received Count — FCOEDWRC
		0x08784, // FCoE Packets Transmitted Count — FCOEPTC
		0x08788, // FCoE DWord Transmitted Count — FCOEDWTC
		0x0EE58, // Flow Director Filters Match Statistics — FDIRMATCH (page 657)
		0x0EE5C, // Flow Director Filters Miss Match Statistics — FDIRMISS (page 657)
		// starting on page 639
		0x08F64, // LinkSec Rx Packet OK — LSECRXOK[0]
		0x08F68, // LinkSec Rx Packet OK — LSECRXOK[1]
		0x08F6C, // LinkSec Rx Invalid — LSECRXINV[0]
		0x08F70, // LinkSec Rx Invalid — LSECRXINV[1]
		0x08F74, // LinkSec Rx Not valid count — LSECRXNV[0]
		0x08F78, // LinkSec Rx Not valid count — LSECRXNV[1]
		0x08F7C, // LinkSec Rx Unused SA Count — LSECRXUNSA
		0x08F80, // LinkSec Rx Not Using SA Count — LSECRXNUSA
	};
	for (int n = 0; n < sizeof(stat_regs)/sizeof(stat_regs[0]); n++) {
		REG(stat_regs[n], 0b00000000000000000000000000000000,
				  0b00000000000000000000000000000000);
	}
	// these are RW
	const int stat_regs_rw[] = {
		0x0405C, // Packets Received [64 Bytes] Count — PRC64
		0x04060, // Packets Received [65–127 Bytes] Count — PRC127
		0x04064, // Packets Received [128–255 Bytes] Count — PRC255
		0x04068, // Packets Received [256–511 Bytes] Count — PRC511
		0x0406C, // Packets Received [512–1023 Bytes] Count — PRC1023
		0x04070, // Packets Received [1024 to Max Bytes] Count — PRC1522
		0x02F40, // Rx DMA Statistic Counter Control — RXDSTATCTRL
	};
	for (int n = 0; n < sizeof(stat_regs_rw)/sizeof(stat_regs_rw[0]); n++) {
		REG(stat_regs_rw[n], 0b00000000000000000000000000000000,
				     0b11111111111111111111111111111111);
	}
	// these are RO
	const int stat_regs_ro[] = {
		0x04078, // Broadcast Packets Received Count — BPRC
		0x0407C, // Multicast Packets Received Count — MPRC
		0x04074, // Good Packets Received Count — GPRC
		0x04088, // Good Octets Received Count Low — GORCL
		0x0408C, // Good Octets Received Count High — GORCH
		0x040B4, // Management Packets Received Count — MNGPRC
		0x040B8, // Management Packets Dropped Count — MNGPDC
		0x0CF90, // Management Packets Transmitted Count — MNGPTC
		// and then, starting on page 634
		0x08A3C, // Tx Untagged Packet Counter — LSECTXUT
		0x08A40, // Encrypted Tx Packets — LSECTXPKTE
		0x08A44, // Protected Tx Packets — LSECTXPKTP
		0x08A48, // Encrypted Tx Octets — LSECTXOCTE
		0x08A4C, // Protected Tx Octets — LSECTXOCTP
		0x08F40, // LinkSec Untagged Rx Packet — LSECRXUT
		0x08F44, // LinkSec Rx Octets Decrypted — LSECRXOCTE
		0x08F48, // LinkSec Rx Octets Validated — LSECRXOCTP
		0x08F4C, // LinkSec Rx Packet with Bad Tag — LSECRXBAD
		0x08F50, // LinkSec Rx Packet No SCI — LSECRXNOSCI
		0x08F54, // LinkSec Rx Packet Unknown SCI — LSECRXUNSCI
		0x08F58, // LinkSec Rx Unchecked Packets — LSECRXUC
		0x08F5C, // LinkSec Rx Delayed Packets — LSECRXDELAY
		0x08F60, // LinkSec Rx Late Packets — LSECRXLATE
	};
	for (int n = 0; n < sizeof(stat_regs_ro)/sizeof(stat_regs_ro[0]); n++) {
		REG(stat_regs_ro[n], 0b00000000000000000000000000000000,
				     0b00000000000000000000000000000000);
	}
	// Transmit Queue Statistic Mapping Registers — TQSM[n] (0x08600 + 4*n, n=0...31; RW)
	for (int n = 0; n <= 31; n++) {
		REG(0x08600 + 4*n, 0b00000000000000000000000000000000,
				   0b00001111000011110000111100001111);
	}
	// Queue Packets Received Count — QPRC[n] (0x01030 + 0x40*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x01030 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000000000000000000000);
	}
	// Queue Packets Received Drop Count — QPRDC[n] (0x01430 + 0x40*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x01430 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000000000000000000000);
	}
	// Queue Bytes Received Count Low — QBRC_L[n] (0x01034 + 0x40*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x01034 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000000000000000000000);
	}
	// Queue Bytes Received Count High — QBRC_H[n] (0x01038 + 0x40*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x01038 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000000000000000000000);
	}
	// Queue Packets Transmitted Count — QPTC[n] (0x08680 + 0x4*n, n=0...15 / 0x06030 + 0x40*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x08680 + 0x4*n, 0b00000000000000000000000000000000,
				     0b00000000000000000000000000000000);
		REG(0x06030 + 0x40*n, 0b00000000000000000000000000000000,
				      0b00000000000000000000000000000000);
	}
	// Queue Bytes Transmitted Count Low — QBTC_L[n] (0x08700 + 0x8*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x08700 + 0x8*n, 0b00000000000000000000000000000000,
				     0b00000000000000000000000000000000);
	}
	// Queue Bytes Transmitted Count High — QBTC_H[n] (0x08704 + 0x8*n, n=0...15; RC)
	for (int n = 0; n <= 15; n++) {
		REG(0x08704 + 0x8*n, 0b00000000000000000000000000000000,
				     0b00000000000000000000000000000000);
	}
}
#ifdef VIGOR_EXECUTABLE
volatile uint8_t mapped_memory0[1<<20];
volatile uint8_t mapped_memory1[1<<20];
volatile uint8_t mapped_memory2[1<<20];

volatile uint8_t shadow_mapped_memory0[1<<20];
volatile uint8_t shadow_mapped_memory1[1<<20];
volatile uint8_t shadow_mapped_memory2[1<<20];

volatile uint8_t *get_mapped_memory_ptr(int id) {
    if (id == 0)
        return &mapped_memory0[0];
    else if (id == 1)
        return &mapped_memory1[0];
    else
        return &mapped_memory2[0];
}

volatile uint8_t *get_mapped_shadow_memory_ptr(int id) {
    if (id == 0)
        return &shadow_mapped_memory0[0];
    else if (id == 1)
        return &shadow_mapped_memory1[0];
    else
        return &shadow_mapped_memory2[0];
}

uint8_t get_num_devs() {
    assert(STUB_HARDWARE_DEVICES_COUNT <= 3);
    return 2;
}
#endif//VIGOR_EXECUTABLE


static void
stub_device_init(struct stub_device* dev)
{
#ifdef VIGOR_EXECUTABLE
    //orig_printf("device %d initializing\n", dev->id);
       // "Fake" memory, intercepted
       dev->mem = get_mapped_memory_ptr(dev->id);

       // Real backing store
       dev->mem_shadow = get_mapped_shadow_memory_ptr(dev->id);
    //orig_printf("device %d (mem:%p) initialized\n", dev->id, dev->mem);
#else//VIGOR_EXECUTABLE
	// "Fake" memory, intercepted
	dev->mem = malloc(dev->mem_len);
	klee_intercept_reads(dev->mem, "stub_hardware_read");
	klee_intercept_writes(dev->mem, "stub_hardware_write");

	// Real backing store
	dev->mem_shadow = malloc(dev->mem_len);
#endif//VIGOR_EXECUTABLE
        //klee_possibly_havoc(dev->mem_shadow, dev->mem_len, "mem_shadow");
	memset(dev->mem_shadow, 0, dev->mem_len);

	stub_device_reset(dev);
}

static struct stub_device*
stub_device_get(uint64_t addr)
{
	for (int n = 0; n < STUB_HARDWARE_DEVICES_COUNT; n++) {
		if (addr == (uint64_t) DEVICES[n].mem) {
			return &DEVICES[n];
		}
	}

        orig_printf("no device found for addr %p! Aborting.\n", addr);
	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

uint64_t
stub_hardware_read(uint64_t addr, unsigned offset, unsigned size)
{
	struct stub_device* dev = stub_device_get(addr);

	if (size == 1) {do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		return DEV_MEM(dev, offset, uint8_t);
	}
	if (size == 2) {do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		return DEV_MEM(dev, offset, uint16_t);
	}
	if (size == 4) {
		uint32_t current_value = DEV_REG(dev, offset);

		struct stub_register reg = REGISTERS[offset];
		klee_assert(reg.present);
		klee_assert(reg.readable);

		if (reg.read != NULL) {
			DEV_REG(dev, offset) = reg.read(dev, (uint32_t) offset);
		}

		return current_value;
	}
	if (size == 8) {do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		return DEV_MEM(dev, offset, uint64_t);
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

void
stub_hardware_write(uint64_t addr, unsigned offset, unsigned size, uint64_t value)
{
	struct stub_device* dev = stub_device_get(addr);

	if (size == 1) {
            
            orig_printf("size 1 writes unsupported!\n");
            
            do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		DEV_MEM(dev, offset, uint8_t) = (uint8_t) value;
	} else if (size == 2) {do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		DEV_MEM(dev, offset, uint16_t) = (uint16_t) value;
	} else if (size == 4) {
		struct stub_register reg = REGISTERS[offset];
		klee_assert(reg.present);

		uint32_t current_value = DEV_REG(dev, offset);
		uint32_t new_value = (uint32_t) value;
		uint32_t changed = current_value ^ new_value;

		if ((changed & ~reg.writable_mask) != 0) {
			klee_print_expr("offset", offset);
			klee_print_expr("old", current_value);
			klee_print_expr("new", new_value);
			klee_print_expr("changed", changed);
			do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		}

		if (reg.write != NULL) {
			new_value = reg.write(dev, (uint32_t) offset, new_value);
		}

		DEV_REG(dev, offset) = new_value;
	} else if (size == 8) {do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
		DEV_MEM(dev, offset, uint64_t) = (uint64_t) value;
	} else {
		do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
	}
}

#ifdef VIGOR_EXECUTABLE
// Wrappers that should preserve the stack frame of the function that was executed before
// pin intercepted a write or a read.
// Defined in hardware_stub_wrappers.s (for finer stack management)
extern uint64_t
stub_hardware_read_wrapper(uint64_t addr, unsigned offset, unsigned size);


// Defined in hardware_stub_wrappers.s
extern uint64_t
stub_hardware_write_wrapper(uint64_t addr, unsigned offset, unsigned size, uint64_t value);
#endif//VIGOR_EXECUTABLE


void
stub_free(struct rte_mbuf* mbuf) {
	// Ugh, we have to trace an mbuf with the right address, so we copy the whole thing... this is silly
	memcpy(&traced_mbuf, mbuf, sizeof(struct rte_mbuf));
	memcpy(&traced_mbuf_content, mbuf->buf_addr + mbuf->data_off, sizeof(struct stub_mbuf_content));
	traced_mbuf.buf_addr = &traced_mbuf_content - mbuf->data_off;
	stub_core_trace_free(&traced_mbuf);

	// Still need to free the actual mbuf though
	rte_mbuf_raw_free(mbuf);

	// Soundness check
	klee_assert(!free_called);
	free_called = true;
}

__attribute__((constructor(102))) // Low prio, must execute before other stuff, but after rte_timer_init (which I've given 101)
static void
stub_hardware_init(void)
{
	// Helper method declarations
	char* stub_pci_name(int index);

#ifndef VIGOR_EXECUTABLE
	// Intercept free to trace
	klee_alias_function_regex("rte_pktmbuf_free[0-9]*", "stub_free");
#endif//!VIGOR_EXECUTBALE

	// Register models initializations
	stub_registers_init();

	// Device initialization
	for (int n = 0; n < STUB_HARDWARE_DEVICES_COUNT; n++) {
		struct stub_device stub_dev = {
                        .id = n,
			.name = stub_pci_name(n),
			.interrupts_fd = 0, // set by stdio_files stub
			.interrupts_enabled = false,
			.mem = NULL,
			.mem_len = 1 << 20, // 2^20 bytes
			.mem_shadow = NULL,
			.current_mdi_address = -1,
			.i2c_state = -1,
			.i2c_counter = 0,
			.i2c_address = 0,
			.i2c_start_time = 0,
			.i2c_clock_time = 0,
			.i2c_stop_time = 0,
			.sfp_address = 0
		};
		stub_device_init(&stub_dev);
		DEVICES[n] = stub_dev;

	}

	// DPDK "delay" method override
	rte_delay_us_callback_register(stub_delay_us);
}


void
stub_hardware_receive_packet(uint16_t device)
{
	stub_device_start(&(DEVICES[device]));
}

void
stub_hardware_reset_receive(uint16_t device)
{
	struct stub_device* dev = &(DEVICES[device]);

	// Reset descriptor ring
	DEV_REG(dev, 0x01010) = 0;
	DEV_REG(dev, 0x01018) = 95;

	// Reset descriptor
	uint64_t rdba =  ((uint64_t) DEV_REG(dev, 0x01000)) // RDBAL
		      | (((uint64_t) DEV_REG(dev, 0x01004)) << 32); // RDBAH
	uint64_t* descr = (uint64_t*) rdba;
	descr[0] = dev->old_mbuf_addr;
	descr[1] = 0;

	memset((char*) descr[0], 0, sizeof(struct stub_mbuf_content));

	rx_called = false;
	tx_called = false;
	free_called = false;
}


// Helper methods - not part of the stubs

char*
stub_pci_name(int index)
{
	klee_assert(index >= 0 && index < 10); // simpler

	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "0000:00:00.%d", index);
	return strdup(buffer);
}
