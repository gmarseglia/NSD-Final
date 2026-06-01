# NSD-Final

# Setup 

## Basic networking

![Base networking - R101](images/basic.png)

### R101

The AS100 routers are FRR routers (`nsdcourse/frr:latest`). They configured via `vytsh` configuration terminal.

The most important steps for R101 are presented below. (It's analogous for R102 and R103)


#### `frr.conf`

1. Set address for the interface toward CE1:
```
interface eth0
 ip address 10.1.1.1/30
exit
```

2. Set addresses for the interfaces toward the other R10x routers in the AS100:
```
interface eth1
 ip address 10.0.11.5/30
exit
!
interface eth2
 ip address 10.0.11.1/30
exit
```

3. Also setup the address for the loobpack interface:
```
interface lo
 ip address 5.170.0.101/32
exit
```

4. Set static route for the link toward CE1:
```
ip route 10.1.1.2/32 eth0
```

5. Setup OSPF for the internal AS100 reachability:
```
router ospf
 ospf router-id 5.170.0.101
 network 5.170.0.101/32 area 0
 network 10.0.11.0/30 area 0
 network 10.0.11.4/30 area 0
exit
```

6. Setup iBGP:
```
router bgp 100
 bgp router-id 5.170.0.101
 neighbor 5.170.0.102 remote-as 100
 neighbor 5.170.0.102 update-source 5.170.0.101
 neighbor 5.170.0.103 remote-as 100
 neighbor 5.170.0.103 update-source 5.170.0.101
 !
 address-family ipv4 unicast
  network 10.1.1.2/32
  neighbor 5.170.0.102 next-hop-self
  neighbor 5.170.0.103 next-hop-self
 exit-address-family
exit
```

### CE1

The CEx devices are Linux routers (`nsdcourse/basenet:latest`). They configured via a `init.sh` script.

The most important steps for CE1 are presented below.

#### `init.sh`
1. Set address for the interface toward R101:
```bash
ip addr add 10.1.1.2/30 dev eth1
```

2. Set the address for the interface toward the VPN Site 1:
```bash
ip addr add 192.168.1.1/24 dev eth0
```

3. Enable forwarding and setup default route:
```bash
echo 1 > /proc/sys/net/ipv4/ip_forward
ip route add default via 10.1.1.1
```

### Client A1

Clients are either simple clients (`nsdcourse/basenet:latest`) or VM (`debian-13.3.0-amd64-netinst`), configured via a `init.sh` script.

The most important steps for Client A1 are presented below.

#### `init.sh`

1. Set address for the interface toward CE1 and setup the default route:
```bash
ip addr add 192.168.1.2/24 dev ens33
ip route add default via 192.168.1.1
```

## OpenVPN

![OpenVPN](images/openvpn.png)

### PKI and certificates

In this setup the PKI overlaps with the OpenVPN hub at CE3.

The most important steps for the creation of all the cryptographic keys and certificates are presented below.

#### `create-pki.sh`

1. Create material:
```bash
EASY_RSA="/usr/share/easy-rsa/easyrsa"

$EASY_RSA init-pki
$EASY_RSA build-ca nopass
$EASY_RSA build-server-full server nopass
$EASY_RSA build-client-full client1 nopass
$EASY_RSA build-client-full client2 nopass
$EASY_RSA gen-dh
```

2. Move material into a specificic directory to be moved:
```bash
PKI="/root/pki"
OPENVPN="/root/openvpn"

# for all
for NAME in server client1 client2
do
    mkdir -p ${OPENVPN}/${NAME}
    cp ${PKI}/ca.crt ${OPENVPN}/$NAME
    cp ${PKI}/issued/${NAME}.crt ${OPENVPN}/${NAME}
    cp ${PKI}/private/${NAME}.key ${OPENVPN}/${NAME}
done

# for server only
cp ${PKI}/dh.pem ${OPENVPN}/server
```
- All share the PKI certificate.
- Each member has its own certificate and private key.
- The server also has Diffie-Hellman parameters.

### Server/Hub - CE3

The most important parts for OpenVPN configuration files (`server.conf`, `ccd/client1`, `ccd/client2`) used by the server OpenVPN daemon in server/hub @ CE3 are presented below.

#### `server.conf`
1. Indicate cryptographical material:
```
ca   /root/openvpn/server/ca.crt
cert /root/openvpn/server/server.crt
key  /root/openvpn/server/server.key
dh   /root/openvpn/server/dh.pem
```

2. Allow clients to communicate with each other:
```
client-to-client
```

3. Push to clients the subnet behind CE3:
```
# Push to clients: the route for server-side
push "route 192.168.3.0 255.255.255.0"
```

4. Push to server the subnets behind CE1 and CE2:
```
# Set on server: the routes for client-side
route 192.168.1.0 255.255.255.0
route 192.168.2.0 255.255.255.0
route 192.168.32.0 255.255.255.0
route 192.168.95.0 255.255.255.0
```

5. Setup `ccd` to distribute routing information per client
``` 
client-config-dir /root/openvpn/ccd
```

#### `ccd/client1`

1. Set the subnet reachability through client1 itself:
```
# Set to OpenVPN: source of the client-side
iroute 192.168.1.0 255.255.255.0
```

2. Push to client1 the all subnets behind the OpenVPN network:
```
# Push to client: the sroute for the other client-side 
push "route 192.168.2.0 255.255.255.0"
push "route 192.168.32.0 255.255.255.0"
push "route 192.168.95.0 255.255.255.0"
```

#### `ccd/client2`

1. Set the subnet reachability through client2 itself:
```
iroute 192.168.2.0 255.255.255.0
iroute 192.168.32.0 255.255.255.0
iroute 192.168.95.0 255.255.255.0
```

2. Push to client2 the all subnets behind the OpenVPN network:
```
push "route 192.168.1.0 255.255.255.0"
```

#### `init.sh`

1. Launch OpenVPN deamon at CE3 startup:
```bash
openvpn --config /root/server.conf --daemon
```

### Client/Spoke - CE1 & CE2

The most important parts for OpenVPN configuration file (`client1.conf`) used by the client OpenVPN daemon in client/spoke @ CE1 is presented below. (For client2 it's analogous)

#### `client1.conf`

1. Set the IP address of the server/hub:
```
remote 10.1.3.2 1194
```

2. Indicate cryptographical material:
```
ca   /root/openvpn/client1/ca.crt
cert /root/openvpn/client1/client1.crt
key  /root/openvpn/client1/client1.key
```

## 802.1x Authentication

![RADIUS](images/radius.png)

### RADIUS server

The server is a RADIUS server (`nsdcourse/radius:latest`). It is configured via the configuration files commonly used by `freeradius` daemon.

The most important parts for RADIUS configuration files (`authorize`, `clients.conf`) used by the RADIUS server are presented below.

#### `clients.conf`

In `/etc/freeradius/3.0/clients.conf` there is the list of RADIUS client devices.

```
client ebpf-switch {
    ipaddr = 192.168.2.2
    secret = "myRadiusSecret"
    shortname = ebpf-switch-1
}
```
- Setups eBPF-Switch as client.

#### `authorize`

In `/etc/freeradius/3.0/mods-config/files/authorize` there is the list of RADIUS users with their credentials.

```conf
clientb1	Cleartext-Password := "clientb1"
			Service-Type = Framed-User,
			Tunnel-Type = 13,
			Tunnel-Medium-Type = 6,
			Tunnel-Private-Group-ID = 32,
			Calling-Station-Id = "%{Calling-Station-Id}"
		
clientb2	Cleartext-Password := "clientb2"
			Service-Type = Framed-User,
			Tunnel-Type = 13,
			Tunnel-Medium-Type = 6,
			Tunnel-Private-Group-ID = 95,
			Calling-Station-Id = "%{Calling-Station-Id}"
```
- `Tunnel-Private-Groud-ID` indicates the vlanID that will be assigned to the user.
- `Calling-Station-Id = "%{Calling-Station-Id}"` allows to get the MAC address of user request in the server response $\rightarrow$ it's later used in the eBPF-Switch to associate `<MAC, interface, vlanID>`. 

### CE2 

CE2 networking configuration, other than the basic networking commands presented earlier, presents some commands to enable VLAN interfaces.

#### `init.sh`

1. Create the virtual interfaces for VLAN:
```bash
ip link add link eth0 name eth0.32 type vlan id 32
ip link add link eth0 name eth0.95 type vlan id 95

ip addr add 192.168.32.1/24 dev eth0.32
ip addr add 192.168.95.1/24 dev eth0.95

ip link set eth0.32 up
ip link set eth0.95 up
```

### eBPF-Switch 

The eBPF-Switch needs to be configured for:
- MAC networking: create and modify `ebtables` rules to allow traffic only from authenticated interfaces.
- IP networking: create the bridge to allow VLAN operations.
- BPF/XDP: attach an XDP program to parse RADIUS response to check authentication result and get relative VLAN ID.
- `hostapd`: act as a RADIUS client and respond to RADIUS events.

The most important parts for these configurations are presented below.

#### `init.sh`

This initial script does the initial configuration.

1. Create the VLAN bridge and attach the relative interfaces:
```bash
# Initialize
ip link add name br-auth type bridge
ip link set dev br-auth type bridge vlan_filtering 1

# Add all relevant interfaces to the bridge, including the uplink (eth0)
ip link set dev eth0 master br-auth
ip link set dev eth1 master br-auth
ip link set dev eth2 master br-auth

# Bring up the physical interfaces and the bridge
ip link set dev eth0 up
ip link set dev eth1 up
ip link set dev eth2 up
ip link set br-auth up

# Assign the IP address and default route to the BRIDGE interface, not eth0
ip addr add 192.168.2.2/24 dev br-auth
ip route add default via 192.168.2.1 dev br-auth

# Forward EAPOL frames (for 802.1X if you are using hostapd)
echo 8 > /sys/class/net/br-auth/bridge/group_fwd_mask
```

2. Setup default `ebtables` rules:
```bash
# MAC rules
ebtables -P FORWARD DROP
ebtables -P INPUT ACCEPT
ebtables -P OUTPUT ACCEPT
```
- By default no packets are forwarded.

3. Setup BPF FS and load the XDP program to parse the RADIUS response:
```bash
make -C /root/xdp/ load
```
- Loads the XDP program: `bpftool prog load xdp_radius.o /sys/fs/bpf/xdp_radius_prog`.
- Attaches it to the `br-auth` bridge: `bpftool net attach xdp pinned /sys/fs/bpf/xdp_radius_prog dev br-auth`.

4. Launches `hostapd` and `hostapd_cli` as background daemons:
```bash
hostapd /etc/hostapd/hostapd.conf -B
echo "" > /root/fwd_table.json
hostapd_cli -a /root/8021x_handler.py -B
```

#### `hostapd.conf`

This configuration file holds the configuration as RADIUS client.

1. Set the `br-auth` as the interface for the authentication request:
```conf
interface=br-auth
driver=wired
```

2. Set the credential for the authentication of itself as RADIUS client:
```conf
auth_server_addr=192.168.3.2
auth_server_port=1812
auth_server_shared_secret=myRadiusSecret
```

#### `xdp_radius.c`

This XDP program parses packets, looking for RADIUS packets. If one it's found, it tries to extract the authenticated association `<MAC, VLAN id>`.

The program has been built following the XDP tutorials on GitHub.

1. Parse Ethernet, IP UDP, and RADIUS headers sequentially:
```c
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

static __always_inline int parse_radiushdr(struct hdr_cursor *nh,
                                           void *data_end,
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

SEC("xdp")
int xdp_parser_func(struct xdp_md *ctx) {
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;
  struct ethhdr *eth;
  struct iphdr *iph;
  struct udphdr *udph;
  struct radiushdr *radh;
  char key[KEY_LEN] = {0};
  char val[VAL_LEN] = {0};

  /* These keep track of the next header type and iterator pointer */
  struct hdr_cursor nh;
  int nh_type;
  int ret;

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

  ...


out:
  /* Default action XDP_PASS, imply everything we couldn't parse, or that
   * we don't want to deal with, we just pass up the stack and let the
   * kernel deal with it.
   */
  return XDP_PASS; /* read via xdp_stats */
}

char _license[] SEC("license") = "GPL";
```

2. If headers checks are passed, then try to extract the `Calling-Station-Id` (MAC address) and `Tunnel-Private-Group-ID` (VLAN ID) from the packet and save them in two buffers. Later, if found, they are saved in a map, with the MAC address as key and the VLAN ID as field:
```c
/* eBPF Limits */
#define MAX_STR_LEN 32
#define KEY_LEN 18
#define VAL_LEN 5
#define MAX_RADIUS_ATTRS 32

/* Define the Pinned Map */
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, char[KEY_LEN]);
  __type(value, char[VAL_LEN]);
  __uint(max_entries, 1024);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} radius_sessions SEC(".maps");

static __always_inline int fill_entry(struct hdr_cursor *nh, void *data_end,
                                      char key[KEY_LEN], char val[VAL_LEN]) {
  __u8 *attr_ptr = nh->pos;
  __u8 found_key = 0;
  __u8 found_val = 0;

  for (int i = 0; i < MAX_RADIUS_ATTRS; i++) {
    if ((void *)attr_ptr + 2 > data_end)
      return -1;

    __u8 attr_type = *attr_ptr;
    __u8 attr_len = *(attr_ptr + 1);

    if (attr_len < 2)
      return -1;

    __u8 *attr_val_ptr = attr_ptr + 2;
    __u8 attr_val_len = attr_len - 2;

    if ((void *)(attr_val_ptr + attr_val_len) > data_end)
      return -1;

    if (attr_type == RADIUS_ATTR_CALLING_STATION_ID) {
      if (attr_val_len > KEY_LEN - 1)
        return -1;
      bpf_probe_read_kernel(key, attr_val_len, attr_val_ptr);
      key[attr_val_len] = '\0';
      found_key = 1;
    } else if (attr_type == RADIUS_ATTR_TUNNEL_PRIVATE_GROUP_ID) {
      if (attr_val_len > VAL_LEN - 1)
        return -1;
      bpf_probe_read_kernel(val, attr_val_len, attr_val_ptr);
      val[attr_val_len] = '\0';
      found_val = 1;
    }

    if (found_key && found_val)
      return 0;

    attr_ptr += attr_len;
  }

  return -1;
}

int xdp_parser_func(struct xdp_md *ctx) {
  ...
  /* Filter only Success messages */
  if (radh->code != RADIUS_CODE_ACCESS_ACCEPT)
    goto out;

  /* Find and fill attributes: CALLING_STATION_ID and TUNNEL_PRIVATE_GROUP_ID
   * from the Radius attributes
   */
  ret = fill_entry(&nh, data_end, key, val);

  if (ret != 0)
    goto out;

  /* If attributes  CALLING_STATION_ID and TUNNEL_PRIVATE_GROUP_ID are found,
   * then update the map
   */
  bpf_map_update_elem(&radius_sessions, key, val, BPF_ANY);

out:
 ...
}
```

#### `8021x_handler.py`

This script is invoked by `hostapd_cli` at every RADIUS event.

1. Receive interface (`br-auth`), event type and associated MAC address:
```python
if __name__ == "__main__":
    interface = sys.argv[1]
    event = sys.argv[2]
    mac = sys.argv[3] if len(sys.argv) > 3 else None

    print(...)

    handle_event(interface, event, mac)
```

2. If the event correspond to a successful authentication (`AP-STA-CONNECTED`) or to a client disconnection (`AP-STA-DISCONNECTED`), then run `ebtables` command to allow/deny the packets from/to the authenticated MAC address:
```python
def handle_event(interface, event, mac=None):
    """Handle the event and update allowed_macs list"""
    ...
    if event in ("AP-STA-CONNECTED") and mac:
        # Run commands
        run_cmd(f"ebtables -A FORWARD -s {mac} -j ACCEPT")
        run_cmd(f"ebtables -A FORWARD -d {mac} -j ACCEPT")
        ...
    elif event in ("AP-STA-DISCONNECTED") and mac:
        run_cmd(f"ebtables -D FORWARD -s {mac} -j ACCEPT")
        run_cmd(f"ebtables -D FORWARD -d {mac} -j ACCEPT")
```

3. Also, get the interface the user MAC is connected to and the VLAN ID associated with it:
```python
def get_interface(mac: str) -> str | None:
    """Get the physical port associated with the given MAC address"""
    result = run_cmd_and_return(f"bridge fdb show | grep {mac}")
    #Parse the result
    ...

def get_vlan_id(mac: str) -> str | None:
    """Get the VLAN ID associated with the given MAC address"""
    result = run_cmd_and_return(f"bpftool map dump name radius_sessions")
    # Parse the result
    ...

def handle_event(interface, event, mac=None):
    """Handle the event and update allowed_macs list"""
    ...
    if event in ("AP-STA-CONNECTED") and mac:
        ...
        port = get_interface(mac)
        vlan_id = get_vlan_id(mac)
        ...
    elif event in ("AP-STA-DISCONNECTED") and mac:
        ...
        port = get_interface(mac)
        vlan_id = get_vlan_id(mac)
        ...
```
- The result of `bridge fdb show` is parsed to get the association `<MAC, interface>`.
- The BPF map readable through `bpftool map dump name radius_sessions` is parsed to get the association `<MAC, VLAN id>`.

4. Run IP commands to setup the `br-auth` bridge:
```python
def handle_event(interface, event, mac=None):
    """Handle the event and update allowed_macs list"""
    ...
    if event in ("AP-STA-CONNECTED") and mac:
        ...
        run_cmd(f"bridge vlan add dev {port} vid {vlan_id} pvid untagged")
        run_cmd(f"bridge vlan add dev eth0 vid {vlan_id}")
        run_cmd(f"bridge vlan add dev br-auth vid {vlan_id} self")
    elif event in ("AP-STA-DISCONNECTED") and mac:
        ...
        run_cmd(f"bridge vlan del dev {port} vid {vlan_id}")
        run_cmd(f"bridge vlan add dev {port} vid 1 pvid untagged")
        run_cmd(f"bpftool map delete name radius_sessions key hex {get_bytes(mac)}")
```
- In case of user disconnection, the entry in the BPF map is also deleted.

### Client-B1

The Client-B1 and B2 are configured to use `wpa_supplicant` to authenticate.

#### `wpa_supplicant.conf`

The `/etc/wpa_supplicant.conf` file holds the credentials for the each client.

```conf
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
ap_scan=0

network={
    key_mgmt=IEEE8021X
    eap=MD5
    identity="clientb1"
    password="clientb1"
    eapol_flags=0
}
```
- It's analogous for Client-B2.

#### `init.sh`

In this script the `wpa_supplicant` is called in order to begin the authentication process.
```bash
wpa_supplicant -c /etc/wpa_supplicant.conf -D wired -i eth0
# wpa_cli -i eth0 logoff
# wpa_cli -i eth0 logon
```
