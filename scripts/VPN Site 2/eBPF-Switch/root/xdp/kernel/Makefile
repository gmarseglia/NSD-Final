CC = clang
CFLAGS = -O2 -g -Wall -target bpf -D__TARGET_ARCH_x86
INCLUDES = -I/usr/include
INCLUDES += -I/workspace/xdp-tutorial/lib/install/include
# INCLUDES += -I/home/giuseppe/NSD/xdp-tutorial/lib/install/include

SRC = xdp_radius.c
OBJ = xdp_radius.o

IFACE ?= br-auth

PIN_DIR = /sys/fs/bpf
PROG_PIN = $(PIN_DIR)/xdp_radius_prog
MAP_PIN = $(PIN_DIR)/radius_sessions

.PHONY: all clean load unload status map-dump

all: $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

load: $(OBJ)
	@echo "Loading program and pinning to $(PROG_PIN)..."
	bpftool prog load $(OBJ) $(PROG_PIN)
	@echo "Attaching to interface $(IFACE)..."
	bpftool net attach xdp pinned $(PROG_PIN) dev $(IFACE)
	@echo "Success! XDP program attached."

unload:
	@echo "Detaching from $(IFACE)..."
	bpftool net detach xdp dev $(IFACE) || true
	@echo "Removing pinned program and map..."
	rm -f $(PROG_PIN) $(MAP_PIN)
	@echo "Cleanup complete."

status:
	@echo "--- XDP Attachment Status on $(IFACE) ---"
	bpftool net show dev $(IFACE)
	@echo "\n--- Pinned Map Status ---"
	bpftool map show pinned $(MAP_PIN)

map-dump:
	bpftool map dump pinned $(MAP_PIN)

clean:
	rm -f $(OBJ)
