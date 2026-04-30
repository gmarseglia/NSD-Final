#!/bin/bash
set -euo pipefail

EASY_RSA="/usr/share/easy-rsa/easyrsa"
PKI="/root/pki"
OPENVPN="/root/openvpn"

$EASY_RSA init-pki
$EASY_RSA build-ca nopass
$EASY_RSA build-server-full server nopass
$EASY_RSA build-client-full client1 nopass
$EASY_RSA build-client-full client2 nopass
$EASY_RSA gen-dh

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

# ccd configuration
mkdir -p ${OPENVPN}/ccd
echo "iroute 192.168.1.0 255.255.255.0" > ${OPENVPN}/ccd/client1
echo "iroute 192.168.2.0 255.255.255.0" > ${OPENVPN}/ccd/client2