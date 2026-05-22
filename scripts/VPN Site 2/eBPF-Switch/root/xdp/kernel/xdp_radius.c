/* 1. Standard types must come first */
#include <asm/types.h>
#include <linux/types.h>
#include <stddef.h>

/* 2. Core BPF and Network headers */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

/* 3. BPF Helpers must come last */
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>

/* RADIUS Constants */
#define RADIUS_PORT 1812
#define RADIUS_CODE_ACCESS_ACCEPT 2
#define RADIUS_ATTR_CALLING_STATION_ID 31
#define RADIUS_ATTR_TUNNEL_PRIVATE_GROUP_ID 81

/* eBPF Limits */
#define MAX_STR_LEN 32
#define KEY_LEN 18
#define VAL_LEN 5
#define MAX_RADIUS_ATTRS 32

struct radiushdr {
  __u8 code;
  __u8 id;
  __be16 len;
  __u8 authenticator[16];
} __attribute__((packed));

/* Header cursor to keep track of current parsing position */
struct hdr_cursor {
  void *pos;
};

static __always_inline int parse_ethhdr(struct hdr_cursor *nh, void *data_end,
                                        struct ethhdr **ethhdr) {
  struct ethhdr *eth = nh->pos;
  int hdrsize = sizeof(*eth);

  /* Byte-count bounds check; check if current pointer + size of header
   * is after data_end.
   */
  if ((void *)(eth + 1) > data_end)
    return -1;

  nh->pos += hdrsize;
  *ethhdr = eth;

  return eth->h_proto; /* network-byte-order */
}

static __always_inline int parse_iphdr(struct hdr_cursor *nh, void *data_end,
                                       struct iphdr **iphdr) {
  struct iphdr *iph = nh->pos;
  int hdrsize;

  if ((void *)(iph + 1) > data_end)
    return -1;

  hdrsize = iph->ihl * 4;
  /* Sanity check packet field is valid */
  if (hdrsize < sizeof(*iph))
    return -1;

  /* Variable-length IPv4 header, need to use byte-based arithmetic */
  if (nh->pos + hdrsize > data_end)
    return -1;

  nh->pos += hdrsize;
  *iphdr = iph;

  return iph->protocol;
}

static __always_inline int parse_udphdr(struct hdr_cursor *nh, void *data_end,
                                        struct udphdr **udphdr) {
  struct udphdr *udph = nh->pos;
  int hdrsize = sizeof(*udph);

  /* Byte-count bounds check; check if current pointer + size of header
   * is after data_end.
   */
  if ((void *)(udph + 1) > data_end)
    return -1;

  nh->pos += hdrsize;
  *udphdr = udph;

  return 0; /* network-byte-order */
}

static __always_inline int parse_radiushdr(struct hdr_cursor *nh, void *data_end,
                                        struct radiushdr **radiushdr) {
  struct radiushdr *radiush = nh->pos;
  int hdrsize = sizeof(*radiush);

  /* Byte-count bounds check; check if current pointer + size of header
   * is after data_end.
   */
  if ((void *)(radiush + 1) > data_end)
    return -1;

  nh->pos += hdrsize;
  *radiushdr = radiush;

  return 0; /* network-byte-order */
}

/* Define the Pinned Map */
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, char[KEY_LEN]);
  __type(value, char[VAL_LEN]);
  __uint(max_entries, 1024);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} radius_sessions SEC(".maps");

SEC("xdp")
int xdp_parser_func(struct xdp_md *ctx) {
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;
  struct ethhdr *eth;
  struct iphdr *iph;
  struct udphdr *udph;
  struct radiushdr *radh;

  /* These keep track of the next header type and iterator pointer */
  struct hdr_cursor nh;
  int nh_type;

  /* Start next header cursor position at data start */
  nh.pos = data;

  /* Packet parsing in steps: Get each header one at a time, aborting if
   * parsing fails. Each helper function does sanity checking (is the
   * header type in the packet correct?), and bounds checking.
   */
  nh_type = parse_ethhdr(&nh, data_end, &eth);
  if (nh_type != bpf_htons(ETH_P_IP))
    goto out;

  /* Parse IPv4 header
   */
  nh_type = parse_iphdr(&nh, data_end, &iph);
  if (nh_type != IPPROTO_UDP)
    goto out;

  /* Parse UDP header
   */
  nh_type = parse_udphdr(&nh, data_end, &udph);
  if (nh_type != 0 || udph->source != bpf_htons(RADIUS_PORT))
    goto out;

  /* Parse RADIUS header
   */
  nh_type = parse_radiushdr(&nh, data_end, &radh);
  if (nh_type != 0)
    goto out;

  /* Filter only Success messages */
  if (radh->code != RADIUS_CODE_ACCESS_ACCEPT)
    goto out;

out:
  /* Default action XDP_PASS, imply everything we couldn't parse, or that
   * we don't want to deal with, we just pass up the stack and let the
   * kernel deal with it.
   */
  return XDP_PASS; /* read via xdp_stats */
}

char _license[] SEC("license") = "GPL";