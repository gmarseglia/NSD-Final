# NSD-Final

# Setup 

## Basic networking

### R101

Starting from the AS100 routers, which are FRR routers (`nsdcourse/frr:latest`) configured via `vytsh` configuration terminal.

The most important steps for R101 are presented below. (It's analogous for R102 and R103)

![Base networking - R101](images/R101.png)

#### `frr.conf`

Set address for the interface toward CE1:
```
interface eth0
 ip address 10.1.1.1/30
exit
```

Set addresses for the interfaces toward the other R10x routers in the AS100:
```
interface eth1
 ip address 10.0.11.5/30
exit
!
interface eth2
 ip address 10.0.11.1/30
exit
```

Also setup the address for the loobpack interface:
```
interface lo
 ip address 5.170.0.101/32
exit
```

Set static route for the link toward CE1:
```
ip route 10.1.1.2/32 eth0
```

Setup OSPF for the internal AS100 reachability:
```
router ospf
 ospf router-id 5.170.0.101
 network 5.170.0.101/32 area 0
 network 10.0.11.0/30 area 0
 network 10.0.11.4/30 area 0
exit
```

Setup iBGP:
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

The CEx devices are Linux routers (`nsdcourse/basenet:latest`), configured via a `init.sh` script.

The most important steps for CE1 are presented below.

![Base networking - CE1](images/CE1.png)

#### `init.sh`
Set address for the interface toward R101:
```bash
ip addr add 10.1.1.2/30 dev eth1
```

Set the address for the interface toward the VPN Site 1:
```bash
ip addr add 192.168.1.1/24 dev eth0
```

Enable forwarding and setup default route:
```bash
echo 1 > /proc/sys/net/ipv4/ip_forward
ip route add default via 10.1.1.1
```

### Client A1

Clients are either simple clients (`nsdcourse/basenet:latest`) or VM (`debian-13.3.0-amd64-netinst`), configured via a `init.sh` script.

The most important steps for Client A1 are presented below.

![Base networking - Client A1](images/client-A1.png)

#### `init.sh`
Set address for the interface toward CE1 and setup the default route:
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

Create material:
```bash
EASY_RSA="/usr/share/easy-rsa/easyrsa"

$EASY_RSA init-pki
$EASY_RSA build-ca nopass
$EASY_RSA build-server-full server nopass
$EASY_RSA build-client-full client1 nopass
$EASY_RSA build-client-full client2 nopass
$EASY_RSA gen-dh
```

Move material into a specificic directory to be moved:
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
Indicate cryptographical material:
```
ca   /root/openvpn/server/ca.crt
cert /root/openvpn/server/server.crt
key  /root/openvpn/server/server.key
dh   /root/openvpn/server/dh.pem
```

Allow clients to communicate with each other:
```
client-to-client
```

Push to clients the subnet behind CE3:
```
# Push to clients: the route for server-side
push "route 192.168.3.0 255.255.255.0"
```

Push to server the subnets behind CE1 and CE2:
```
# Set on server: the routes for client-side
route 192.168.1.0 255.255.255.0
route 192.168.2.0 255.255.255.0
route 192.168.32.0 255.255.255.0
route 192.168.95.0 255.255.255.0
```

Setup `ccd` to distribute routing information per client
``` 
client-config-dir /root/openvpn/ccd
```

#### `ccd/client1`

Set the subnet reachability through client1 itself:
```
# Set to OpenVPN: source of the client-side
iroute 192.168.1.0 255.255.255.0
```

Push to client1 the all subnets behind the OpenVPN network:
```
# Push to client: the sroute for the other client-side 
push "route 192.168.2.0 255.255.255.0"
push "route 192.168.32.0 255.255.255.0"
push "route 192.168.95.0 255.255.255.0"
```

#### `ccd/client2`

Set the subnet reachability through client2 itself:
```
iroute 192.168.2.0 255.255.255.0
iroute 192.168.32.0 255.255.255.0
iroute 192.168.95.0 255.255.255.0
```

Push to client2 the all subnets behind the OpenVPN network:
```
push "route 192.168.1.0 255.255.255.0"
```

#### `init.sh`

Launch OpenVPN deamon at CE3 startup:
```bash
openvpn --config /root/server.conf --daemon
```

### Client/Spoke - CE1 & CE2

The most important parts for OpenVPN configuration file (`client1.conf`) used by the client OpenVPN daemon in client/spoke @ CE1 is presented below. (For client2 it's analogous)

#### `client1.conf`

Set the IP address of the server/hub:
```
remote 10.1.3.2 1194
```

Indicate cryptographical material:
```
ca   /root/openvpn/client1/ca.crt
cert /root/openvpn/client1/client1.crt
key  /root/openvpn/client1/client1.key
```
