import threading
import time
import socket
import os

from common import LOGS_DIR, get_times, send_leader, start_node
from threading import Thread

# Configuration
NODES = 3
GET_TIME_INTERVAL = 2  # seconds
TIMES_LOG = os.path.join(LOGS_DIR, "times.txt")

def main():
    os.makedirs(LOGS_DIR, exist_ok=True)
    open(TIMES_LOG, "w").close()  # clear the file.

    base_port = 12_000
    node_ports = [base_port + i for i in range(NODES)]
    node_processes = []
    targets = []

    print("Running nodes...")
    for i in range(NODES):
        node = start_node(port=node_ports[i]) if i == 0 else \
               start_node(port=node_ports[i], peer_address="localhost", peer_port=node_ports[i - 1]) 
        targets.append(("localhost", node_ports[i]))

        node_processes.append(node)
        time.sleep(1)

    print("Run the listener...")
    stop_event = threading.Event()
    listener_thread = Thread(target=get_times, args=(targets, TIMES_LOG, 5, stop_event))
    listener_thread.start()

    time.sleep(1)
    send_leader(targets[0][0], targets[0][1], 0)

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
