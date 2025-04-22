import threading
import time
import os

from common import LOGS_DIR, get_times, send_leader, start_node
from threading import Thread

"""
    In this test, there are multiple groups of nodes.
    Each group has one leader.
    Each node gets a reference to a previous node inside its group.

    Expected behaviour:
        We have multiple groups synchronized with their respective leaders.
"""

# Configuration
NODES = 13
GROUPS = 3
GET_TIME_INTERVAL = 2  # seconds
TIMES_LOG = os.path.join(LOGS_DIR, "times.txt")

def get_group_sizes(total, groups):
    base = total // groups
    remainder = total % groups
    return [base] * (groups - 1) + [base + remainder]

def main():
    os.makedirs(LOGS_DIR, exist_ok=True)
    open(TIMES_LOG, "w").close()  # clear the file.

    base_port = 12_000
    node_ports = [base_port + i for i in range(NODES)]
    node_processes = []
    targets = []

    group_sizes = get_group_sizes(NODES, GROUPS)
    print(f"Group sizes: {group_sizes}")

    print("Running nodes...")

    port_index = 0
    for group_size in group_sizes:
        group_ports = node_ports[port_index : port_index + group_size]

        for i, port in enumerate(group_ports):
            if i == 0:
                node = start_node(port=port)
            else:
                node = start_node(
                    port=port,
                    peer_address="localhost",
                    peer_port=group_ports[i - 1]
                )
            node_processes.append(node)
            targets.append(("localhost", port))
            time.sleep(1)

        port_index += group_size

    print("Run the listener...")
    stop_event = threading.Event()
    listener_thread = Thread(target=get_times, args=(targets, TIMES_LOG, GET_TIME_INTERVAL, stop_event))
    listener_thread.start()

    time.sleep(1)

    print("Electing leaders...")
    port_index = 0
    for group_size in group_sizes:
        leader_port = node_ports[port_index]
        send_leader("localhost", leader_port, 0)
        port_index += group_size

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass

    print("Terminating processes...")
    for p in node_processes:
        p.terminate()
        p.wait()
        print(f"exit code: {p.returncode}")

    stop_event.set()
    listener_thread.join()
    print("Test finished.")

if __name__ == "__main__":
    main()
