#!/bin/python3
import json
import subprocess
import sys
CONFIG_FILE = "fwd_table.json"

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

def handle_event(interface, event, mac=None):
    """ Handle the event and update allowed_macs list """
    config = load_config()
    if event in ("AP-STA-CONNECTED", "CTRL-EVENT-EAP-SUCCESS", "CTRL-EVENT-EAP-SUCCESS2") and mac:
        if mac not in config["allowed_macs"]:
            config["allowed_macs"].append(mac)
            run_ebtables_cmd(f"ebtables -A FORWARD -s {mac} -j ACCEPT")
            run_ebtables_cmd(f"ebtables -A FORWARD -d {mac} -j ACCEPT")
            save_config(config)
        print(f"{interface}: Allowed traffic from {mac}")
    elif event in ("AP-STA-DISCONNECTED", "CTRL-EVENT-EAP-FAILURE") and mac:
        if mac in config["allowed_macs"]:
            config["allowed_macs"].remove(mac)
            run_ebtables_cmd("ebtables -D FORWARD -s {mac}")
            run_ebtables_cmd("ebtables -D FORWARD -d {mac}")
            save_config(config)
        print(f"{interface}: Denied traffic from {mac}")
    else:
        print(f"Unhadled event: {event}, ignoring...")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(1)
        interface = sys.argv[1]
        event = sys.argv[2]
        mac = sys.argv[3] if len(sys.argv) > 3 else None
        handle_event(interface, event, mac)