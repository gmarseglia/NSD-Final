#!/bin/python3
import json
import re
import subprocess
import sys
CONFIG_FILE = "fwd_table.json"

def is_valid_mac(mac):
    """Check if the given string is a valid MAC address."""
    if not mac:
        return False
    mac_regex = re.compile(r'^([0-9a-fA-F]{2}[:-]){5}([0-9a-fA-F]{2})$')
    return mac_regex.match(mac) is not None

def load_config():
    """ Load the allowed_macs from a JSON file """
    try:
        with open(CONFIG_FILE, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        return {"allowed_macs": []}

def save_config(config):
    """ Save the allowed_macs to a JSON file """
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f, indent=4)

def run_ebtables_cmd(command):
    """ Helper function to run ebtables commands """
    try:
        subprocess.run(command, check=True, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running ebtables command: {e}")

def run_cmd_and_return(command):
    """ Helper function to run ebtables commands """
    try:
        return subprocess.run(command, check=True, shell=True, text=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running ebtables command: {e}")

def get_physical_port(mac):
    result = result = run_cmd_and_return(f"bridge fdb show | grep {mac}")
    for line in result.stdout.splitlines():
            # The line format is usually: <mac> dev <interface> master bridge ...
            parts = line.split()
            if 'dev' in parts:
                dev_index = parts.index('dev')
                return parts[dev_index + 1] # Returns 'eth1' or 'eth0'

def handle_event(interface, event, mac=None):
    """ Handle the event and update allowed_macs list """
    config = load_config()
    if event in ("AP-STA-CONNECTED") and mac:
        if mac not in config["allowed_macs"]:
            config["allowed_macs"].append(mac)
            run_ebtables_cmd(f"ebtables -A FORWARD -s {mac} -j ACCEPT")
            run_ebtables_cmd(f"ebtables -A FORWARD -d {mac} -j ACCEPT")
            run_ebtables_cmd(f"bridge vlan add dev {get_physical_port(mac)} vid 32 pvid untagged")
            run_ebtables_cmd(f"bridge vlan add dev eth0 vid 32")
            run_ebtables_cmd(f"bridge vlan add dev br-auth vid 32 self")
            save_config(config)
        print(f"{interface}: Allowed traffic from {mac}")
    elif event in ("AP-STA-DISCONNECTED") and mac:
        if mac in config["allowed_macs"]:
            config["allowed_macs"].remove(mac)
            run_ebtables_cmd(f"ebtables -D FORWARD -s {mac} -j ACCEPT")
            run_ebtables_cmd(f"ebtables -D FORWARD -d {mac} -j ACCEPT")
            run_ebtables_cmd(f"bridge vlan del dev {get_physical_port(mac)} vid 32")
            run_ebtables_cmd(f"bridge vlan add dev {get_physical_port(mac)} vid 1 pvid untagged")
            save_config(config)
        print(f"{interface}: Denied traffic from {mac}")
    else:
        print(f"Unhadled event: {event}, ignoring...")

if __name__ == "__main__":
    interface = sys.argv[1]
    event = sys.argv[2]
    mac = sys.argv[3] if len(sys.argv) > 3 else None

    print(f"\nGot sys.argv[1]/interface: {interface}, sys.argv[2]/event: {event}, sys.argv[3]/mac: {mac}", end="")
    if mac and is_valid_mac(mac):
        print(f", port: {get_physical_port(mac)}", end="")
    print("")
    
    handle_event(interface, event, mac)