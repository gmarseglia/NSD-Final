#!/bin/bash

RX_DIR=/root/rx
TX_DIR=/root/tx

# Initialization
rm -rf $RX_DIR $TX_DIR
mkdir -p $RX_DIR $TX_DIR
echo "I'm secret ;)" > $TX_DIR/secret.txt
echo "I'm malicious >:(" > $TX_DIR/malicious.conf

# 1. User `alice` tries to download `/home/alice/secret.txt`
curl -u alice:alice -o $RX_DIR/secret.txt ftp://192.168.1.2//home/alice/secret.txt

# 2. User `alice` tries to upload `/home/alice/secret.txt`
curl -u alice:alice -T $TX_DIR/secret.txt ftp://192.168.1.2//home/alice/secret.txt

# 3. User `root` tries to download `/etc/shadow`
curl -u root:root -o $RX_DIR/shadow ftp://192.168.1.2//etc/shadow

# 4. User `root` tries to upload to `/etc/malicious.conf`
curl -u root:root -T $TX_DIR/malicious.conf ftp://192.168.1.2//etc/malicious.conf

# 5. User `alice` tries to download `/home/alice/.ssh/id_ed25519`
curl -u alice:alice -o $RX_DIR/id_ed25519 ftp://192.168.1.2//home/alice/.ssh/id_ed25519