#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_STR_LEN 32
#define PINNED_MAP_PATH "/sys/fs/bpf/radius_sessions"

/* These structs must exactly match the ones in your XDP program */
struct radius_map_key {
  char val[MAX_STR_LEN];
};

struct radius_map_val {
  char val[MAX_STR_LEN];
};

int main() {
  int map_fd;
  struct radius_map_key *prev_key = NULL;
  struct radius_map_key next_key;
  struct radius_map_val value;

  /* 1. Get a file descriptor to the pinned map */
  map_fd = bpf_obj_get(PINNED_MAP_PATH);
  if (map_fd < 0) {
    fprintf(stderr, "Error: Failed to open pinned map at %s (run as root?)\n",
            PINNED_MAP_PATH);
    return 1;
  }

  printf("%-20s | %-20s\n", "Calling-Station-Id", "Tunnel-Group-ID");
  printf("------------------------------------------------------\n");

  /* 2. Iterate through the hash map */
  int count = 0;
  while (bpf_map_get_next_key(map_fd, prev_key, &next_key) == 0) {

    /* Fetch the value for the current key */
    if (bpf_map_lookup_elem(map_fd, &next_key, &value) == 0) {
      /*
       * Note: We use %.31s to ensure safe printing in case the string
       * somehow isn't perfectly null-terminated in kernel space.
       */
      printf("%-20.31s | %-20.31s\n", next_key.val, value.val);
      count++;
    }

    /* Update prev_key to point to next_key for the next iteration */
    prev_key = &next_key;
  }

  if (count == 0) {
    printf("(Map is empty)\n");
  } else {
    printf("------------------------------------------------------\n");
    printf("Total sessions found: %d\n", count);
  }

  close(map_fd);
  return 0;
}