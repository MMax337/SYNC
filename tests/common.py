import subprocess
import os
import socket
from enum import IntEnum
from pathlib import Path
import sys
import threading
from time import sleep

PROGRAM_NAME = './peer-time-sync'
LOGS_DIR = "LOGS"
Path(LOGS_DIR).mkdir(exist_ok=True)


class Message(IntEnum):
    LEADER = 21
    GET_TIME = 31
    TIME = 32

def start_node(port: int, bind_address: str = None, peer_address: str = None, peer_port: int = None) -> subprocess.Popen:
    args = [f'../{PROGRAM_NAME}']
    if bind_address:
        args += ['-b', bind_address]
    args += ['-p', str(port)]
    if peer_address and peer_port:
        args += ['-a', peer_address, '-r', str(peer_port)]

    log_path = os.path.join(LOGS_DIR, f'log_{port}.txt')
    err_path = os.path.join(LOGS_DIR, f'error_{port}.txt')

    log_file = open(log_path, 'w')
    error_file = open(err_path, 'w')

    process = subprocess.Popen(args, stdout=log_file, stderr=error_file)
    return process

def send_leader(host: str, port: int, synchronized: int):
    if synchronized not in (0, 255):
        raise ValueError("Wartość synchronized musi być 0 lub 255.")
    packet = bytes([Message.LEADER, synchronized])
    print(f"sending: {packet}  ({host}, {port}) ")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(packet, (host, port))

def send_get_time(host: str, port: int):
    packet = bytes([Message.GET_TIME])
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(packet, (host, port))

def get_times(
    targets: list[tuple[str, int]],
    out_file: str,
    send_interval: int,
    stop_event: threading.Event,
    timeout: float = 2.0
):
    """
    Periodically sends a UDP GET_TIME message to a list of (host, port) targets
    and stores their responses to the output file

    Parameters:
        targets        - List of (host, port) tuples
        out_file       - File where responses are stored
        send_interval  - How often to send packets (in seconds)
        timeout        - Max time to wait for a response per target (in seconds)
    """
    open(out_file, "w").close()
    with open(out_file, "a") as file:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(timeout)

            while not stop_event.is_set():
                packet = bytes([Message.GET_TIME])
                for host, port in targets:
                    if stop_event.is_set(): 
                        break
                    try:
                        sock.sendto(packet, (host, port))
                    except socket.timeout:
                        print(f"Timeout on send to {host}:{port}", file=sys.stderr)

                file.write("-----------------------------------------\n")
                for _ in range(len(targets)):   
                    if stop_event.is_set(): 
                        break             
                    try:
                        data, addr = sock.recvfrom(4096)
                        if not data or data[0] != Message.TIME or len(data) != 10:
                            hex_data = ' '.join(f'{byte:02x}' for byte in data)
                            file.write(f"Wrong message from {addr}, message: '{hex_data}'\n")
                        else:
                            timestamp = int.from_bytes(data[2:], byteorder='big')
                            file.write(f"Got from: {addr}, sync_level: {data[1]} time: {timestamp} ms.\n")

                    except socket.timeout:
                        print(f"Timeout on recvfrom", file=sys.stderr)
                        
                file.flush()
                sleep(send_interval)


def get_booted_pcs():
    """
        Runs the command `lk_booted_pcs`.
        Expects the output of the form:
        +--------------+----------------+
        | Name         | Address        |
        +--------------+----------------+
        | red12        | 10.1.1.32      |
        | pink03       | 10.1.1.41      |
        +--------------+----------------+

        Returns two dictionaries
    """
    result = subprocess.run(['lk_booted_pcs'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print(f"Error running command: {result.stderr}")
        return {}

    data = result.stdout.strip().split('\n')

    name_address = {}
    address_name = {}

    for line in data:
        line = line.strip()
        if not line or line.startswith('+') or line.startswith('-'):
            continue
        parts = [part.strip() for part in line.split('|') if part.strip() != '']

        if len(parts) == 2 and parts[0] != "Name":
            name, address = parts
            name_address[name] = address
            address_name[address] = name