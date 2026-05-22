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

struct radius_hdr {
  __u8 code;
  __u8 id;
  __be16 len;
  __u8 authenticator[16];
} __attribute__((packed));

/* Define the Pinned Map */
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, char[KEY_LEN]);
  __type(value, char[VAL_LEN]);
  __uint(max_entries, 1024);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} radius_sessions SEC(".maps");

SEC("xdp")
int xdp_radius_parser(struct xdp_md *ctx) {
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;

  /* 1. Parse Ethernet Header */
  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end)
    return XDP_PASS;

  if (eth->h_proto != bpf_htons(ETH_P_IP))
    return XDP_PASS;

  /* 2. Parse IPv4 Header (Enforce 20-byte static header for safety) */
  struct iphdr *ip = (void *)(eth + 1);
  if ((void *)(ip + 1) > data_end)
    return XDP_PASS;

  if (ip->protocol != IPPROTO_UDP || ip->ihl != 5)
    return XDP_PASS;

  /* 3. Parse UDP Header */
  struct udphdr *udp = (void *)(ip + 1);
  if ((void *)(udp + 1) > data_end)
    return XDP_PASS;

  if (udp->source != bpf_htons(RADIUS_PORT))
    return XDP_PASS;

  /* 4. Parse RADIUS Header */
  struct radius_hdr *rad = (void *)(udp + 1);
  if ((void *)(rad + 1) > data_end)
    return XDP_PASS;

  if (rad->code != RADIUS_CODE_ACCESS_ACCEPT)
    return XDP_PASS;

  /* Phase variables - Use stack primitives, NOT packet pointers */
  char key[KEY_LEN] = {0};
  char val[VAL_LEN] = {0};
  __u8 found_key = 0;
  __u8 found_val = 0;
  __u8 *attr_ptr = (__u8 *)(rad + 1);

  /* =========================================
   * PHASE 1: SEARCH & ISOLATE TO STACK
   * ========================================= */
  for (int i = 0; i < MAX_RADIUS_ATTRS; i++) {
    /* 1. Header bounds check */
    if (attr_ptr + 2 > (__u8 *)data_end)
      break;

    __u8 attr_type = attr_ptr[0];
    __u8 attr_len = attr_ptr[1];
    __u8 *attr_val_ptr = attr_ptr + 2;
    __u8 attr_val_len = attr_len - 2;

    /* 2. Full attribute bounds check */
    if (attr_ptr + attr_len > (__u8 *)data_end)
      break;

    /* 3. Extract immediately to stack if found */
    if (attr_type == RADIUS_ATTR_CALLING_STATION_ID) {
      // Strict check before copy makes the verifier happy immediately
      if (attr_val_ptr + KEY_LEN <= (__u8 *)data_end) {
        __builtin_memcpy(key, attr_val_ptr, KEY_LEN);
        if (attr_val_len >= KEY_LEN)
          key[KEY_LEN - 1] = '\0';
        else
          key[attr_val_len] = '\0';
        found_key = 1;
      }
    } else if (attr_type == RADIUS_ATTR_TUNNEL_PRIVATE_GROUP_ID) {
      if (attr_val_ptr + VAL_LEN <= (__u8 *)data_end) {
        __builtin_memcpy(val, attr_val_ptr, VAL_LEN);
        if (attr_val_len >= VAL_LEN)
          val[VAL_LEN - 1] = '\0';
        else
          val[attr_val_len] = '\0';
        found_val = 1;
      }
    }

    /* 4. Early exit if we have both */
    if (found_key && found_val)
      break;

    /* 5. Jump to next attribute */
    attr_ptr += attr_len;
  }

  /* =========================================
   * PHASE 2: MAP UPDATE (Flat execution path)
   * ========================================= */
  // The verifier loves this because 'key' and 'val' are strictly stack memory
  // now, meaning no complex packet boundary math is required here.
  if (found_key && found_val) {
    bpf_map_update_elem(&radius_sessions, key, val, BPF_ANY);
  }

  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";