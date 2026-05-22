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
    mac_regex = re.compile(r"^([0-9a-fA-F]{2}[:-]){5}([0-9a-fA-F]{2})$")
    return mac_regex.match(mac) is not None


def load_config():
    """Load the allowed_macs from a JSON file"""
    try:
        with open(CONFIG_FILE, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        return {"allowed_macs": []}


def save_config(config):
    """Save the allowed_macs to a JSON file"""
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f, indent=4)


def run_cmd(command):
    """Helper function to run ebtables commands"""
    try:
        subprocess.run(command, check=True, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running ebtables command: {e}")


def run_cmd_and_return(command) -> str:
    """Helper function to run ebtables commands"""
    try:
        return subprocess.run(
            command, check=True, shell=True, text=True, capture_output=True
        ).stdout
    except subprocess.CalledProcessError as e:
        print(f"Error running ebtables command: {e}")
    return ""


def get_physical_port(mac: str) -> str | None:
    """Get the physical port associated with the given MAC address"""
    result = run_cmd_and_return(f"bridge fdb show | grep {mac}")
    for line in result.splitlines():
        # The line format is usually: <mac> dev <interface> master bridge ...
        parts = line.split()
        if "dev" in parts:
            dev_index = parts.index("dev")
            return parts[dev_index + 1]  # Returns 'eth1' or 'eth0'


def get_vlan_id(mac: str) -> str | None:
    """Get the VLAN ID associated with the given MAC address"""
    result = run_cmd_and_return(f"bpftool map dump name radius_sessions")
    # Get the association dictionary
    parsed_list = json.loads(result)
    map = {item["key"]: item["value"] for item in parsed_list}
    # Format the MAC address
    mac_fmt = mac.replace(":", "-").upper()
    # Return the associated VLAN if existing
    if map.get(mac_fmt) is not None:
        return map[mac_fmt]
    else:
        return None


def handle_event(interface, event, mac=None):
    """Handle the event and update allowed_macs list"""
    config = load_config()
    if event in ("AP-STA-CONNECTED") and mac:
        port = get_physical_port(mac)
        vlan_id = get_vlan_id(mac)
        if mac not in config["allowed_macs"]:
            # Save config
            config["allowed_macs"].append(mac)
            save_config(config)
            # Run commands
            run_cmd(f"ebtables -A FORWARD -s {mac} -j ACCEPT")
            run_cmd(f"ebtables -A FORWARD -d {mac} -j ACCEPT")
            run_cmd(f"bridge vlan add dev {port} vid {vlan_id} pvid untagged")
            run_cmd(f"bridge vlan add dev eth0 vid {vlan_id}")
            run_cmd(f"bridge vlan add dev br-auth vid {vlan_id} self")
        print(f"{interface}: Allowed traffic from {mac}")
    elif event in ("AP-STA-DISCONNECTED") and mac:
        if mac in config["allowed_macs"]:
            # Save config
            config["allowed_macs"].remove(mac)
            save_config(config)
            # Run commands
            port = get_physical_port(mac)
            vlan_id = get_vlan_id(mac)
            mac_fmt = mac.replace(":", "-").upper()
            run_cmd(f"ebtables -D FORWARD -s {mac} -j ACCEPT")
            run_cmd(f"ebtables -D FORWARD -d {mac} -j ACCEPT")
            run_cmd(f"bridge vlan del dev {port} vid {vlan_id}")
            run_cmd(f"bridge vlan add dev {port} vid 1 pvid untagged")
            # TODO: fix 
            # run_cmd(f"bpftool map del name radius_sessions {mac_fmt}")
        print(f"{interface}: Denied traffic from {mac}")
    else:
        print(f"Unhadled event: {event}, ignoring...")


if __name__ == "__main__":
    interface = sys.argv[1]
    event = sys.argv[2]
    mac = sys.argv[3] if len(sys.argv) > 3 else None

    print(
        f"\nGot sys.argv[1]/interface: {interface}, sys.argv[2]/event: {event}",
        end="",
    )
    if mac and is_valid_mac(mac):
        print(
            f", sys.argv[3]/mac: {mac}, port: {get_physical_port(mac)}, vlan_id: {get_vlan_id(mac)}",
            end="",
        )
    print("")

    handle_event(interface, event, mac)
