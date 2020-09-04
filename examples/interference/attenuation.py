import shlex
import subprocess
from subprocess import PIPE, TimeoutExpired, STDOUT
import io
import datetime
import threading
from threading import Thread
import time
import os
import atexit
import sys
import queue
import errno
import csv
import math
import re

ON_POSIX = 'posix' in sys.builtin_module_names

class att_experiment:
    def __init__(self, tx_count, phy):
        if (tx_count <= 0):
            raise ValueError("Amount of transmissions can't be negative or zero")
        self.tx_count = tx_count
        self.rx_count = 0
        self.rssi_sum = 0
        self.phy = phy

    def get_tx_count(self):
        return self.tx_count

    def get_rx_count(self):
        return self.rx_count

    def set_rx_count(self, amount):
        if (amount < 0):
            raise ValueError("Can't process a negative amount of receptions")
        elif (amount > self.tx_count):
            raise ValueError("Can't process an amount of receptions > transmissions")

        self.rx_count = amount

    def add_rssi(self, amount):
        self.rssi_sum += amount

    def get_rssi(self):
        if (self.rx_count == 0):
            return 0.0
            
        return self.rssi_sum / self.rx_count

    def packet_success(self):
        if (self.rx_count == 0):
            return 0.0
        
        return self.rx_count / self.tx_count

    def packet_loss(self):
        return 1.0 - self.packet_success()

    def get_phy(self):
        return self.phy

class Queue(queue.Queue):
    def clear(self):
        with self.mutex:
            unfinished = self.unfinished_tasks - len(self.queue)
            if unfinished <= 0:
                if unfinished < 0:
                    raise ValueError('task_done() called too many times')
                self.all_tasks_done.notify_all()
            self.unfinished_tasks = unfinished
            self.queue.clear()
            self.not_full.notify_all()

def enqueue_output(out, queue):
    try:
        for line in iter(out.readline, b''):
            queue.put(line)
        out.close()
    except ValueError: 
        # NOTE an exception is thrown when trying to read a line from out when the
        # subproccess to which this stream belongs was previously killed in another
        # thread
        pass

def exit_handler():
    if isinstance(timing_shell, subprocess.Popen):
        timing_shell.kill()
    if isinstance(rx_shell, subprocess.Popen):
        rx_shell.kill()
    if isinstance(tx_shell, subprocess.Popen):
        tx_shell.kill()
    print("Exiting")

atexit.register(exit_handler)

# NOTE changes the following set of values before starting the script in order
# to reflect the correct scenario
# trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2")]
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
               (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
trx_payload_sizes = [255] # in bytes
trx_dest_addr = "22:68:31:23:2F:4A:16:3A"
attenuation = 39 # in dB
num_of_tx = 10
RIOT_location = "/home/relsas/RIOT-benpicco"
test_duration = int(round(0.5 * num_of_tx)) + 2 # in seconds

LOG_REGEXP_PKT = re.compile(r"^.*?rssi: (?P<rssi>[+-]?\d+).*?")
LOG_REGEXP_PHY = re.compile(r"^.*?PHY")
experiment_index = 0

halt_event = threading.Event()

# NOTE you might have to change the serial port numbers of the devices
# depending on the order in which you plugged them into the USB ports
timing_cmd = "make term PORT=/dev/ttyUSB8 BOARD=remote-revb -C %s/examples/timing_control/" % RIOT_location
rx_cmd = "make term PORT=/dev/ttyUSB1 BOARD=openmote-b"
tx_cmd = "make term PORT=/dev/ttyUSB5 BOARD=openmote-b"

for trx_payload_size in trx_payload_sizes:
    for trx_idx, trx_phy in trx_phy_cfg:

        threading.Timer(1, halt_event.set).start()
        while True:
            if halt_event.is_set():
                halt_event.clear()
                break
        
        tx_shell = subprocess.Popen(shlex.split(tx_cmd),stdin=PIPE,universal_newlines=True)
        try:
            tx_shell.communicate(input="numbytesub %s\n" % (trx_payload_size - 19),timeout=2)
        except TimeoutExpired:
            tx_shell.kill()

        threading.Timer(1, halt_event.set).start()
        while True:
            if halt_event.is_set():
                halt_event.clear()
                break
        
        tx_shell = subprocess.Popen(shlex.split(tx_cmd),stdin=PIPE,universal_newlines=True)
        try:
            tx_shell.communicate(input="saddrsub %s\n" % (trx_dest_addr),timeout=2)
        except TimeoutExpired:
            tx_shell.kill()

        threading.Timer(1, halt_event.set).start()
        while True:
            if halt_event.is_set():
                halt_event.clear()
                break
        
        tx_shell = subprocess.Popen(shlex.split(tx_cmd),stdin=PIPE,universal_newlines=True)
        try:
            tx_shell.communicate(input="physub %s\n" % (trx_idx),timeout=2)
        except TimeoutExpired:
            tx_shell.kill()

        # NOTE an additional waiting period is not needed here because these 2 consecutive shell
        # commands influence the configuration of 2 seperate nodes (transmitter & receiver)

        rx_shell = subprocess.Popen(shlex.split(rx_cmd),stdin=PIPE,universal_newlines=True)
        try:
            rx_shell.communicate(input="physub %s\n" % (trx_idx),timeout=2)
        except TimeoutExpired:
            rx_shell.kill()

        # NOTE an additional waiting period is not needed here because these 2 consecutive shell
        # commands influence the configuration of 2 seperate nodes (receiver & timing controller)
        
        timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
        try:
            timing_shell.communicate(input="numtx %s\n" % (num_of_tx),timeout=2)
        except TimeoutExpired:
            timing_shell.kill()

        threading.Timer(1, halt_event.set).start()
        while True:
            if halt_event.is_set():
                halt_event.clear()
                break

        timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
        try:
            timing_shell.communicate(input="start\n",timeout=2)
        except TimeoutExpired:
            timing_shell.kill()

        rx_log_filename = "rx_log_%s.log" % datetime.datetime.now().strftime("%d-%m-%Y_%H-%M-%S-%f")
        rx_logfile = open(rx_log_filename,"w",newline='')
        rx_logfile.write("Created logfile %s\n" % (rx_log_filename))

        rx_shell = subprocess.Popen(shlex.split(rx_cmd),stdin=PIPE,stdout=PIPE,stderr=PIPE,universal_newlines=True,bufsize=1,close_fds=ON_POSIX)

        rx_q = Queue()
        rx_t = Thread(target=enqueue_output, args=(rx_shell.stdout, rx_q))
        rx_t.daemon = True
        rx_t.start()

        threading.Timer(test_duration + 3, halt_event.set).start()
        while True:
            try:  
                rx_line = rx_q.get_nowait()
            except queue.Empty:
                if halt_event.is_set():
                    halt_event.clear()
                    break
            else: # got line
                # NOTE breaks when reading an empty line and the subprocess has terminated
                if rx_line == '' and rx_shell.poll() is not None:
                    break
                if rx_line != '':
                    rx_logfile.write("%s\n" % (rx_line.strip().strip("\r\n")))
        
        if halt_event.is_set():
            halt_event.clear()
        rx_logfile.write("PHY\n")
        rx_logfile.close()

        timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
        try:
            timing_shell.communicate(input="reboot\n",timeout=2)
        except TimeoutExpired:
            timing_shell.kill()
        
        threading.Timer(2, halt_event.set).start()
        while True:
            if halt_event.is_set():
                halt_event.clear()
                break

        tx_shell = subprocess.Popen(shlex.split(tx_cmd),stdin=PIPE,universal_newlines=True)
        try:
            tx_shell.communicate(input="reboot\n",timeout=2)
        except TimeoutExpired:
            tx_shell.kill()

        try:
            rx_shell.communicate(input="reboot\n",timeout=2)
        except TimeoutExpired:
            rx_shell.kill()

        csv_filename = "./TX_%dB_AT_%dDB.csv" % (trx_payload_size,attenuation)
        current_att_experiment = att_experiment(num_of_tx, trx_phy)

        with open(rx_log_filename, "r") as rx_log:
            for line in rx_log:

                chomped_line = line.rstrip()
                pkt_match = re.match(LOG_REGEXP_PKT, chomped_line)
                phy_match = re.match(LOG_REGEXP_PHY, chomped_line)

                if (pkt_match):
                    rssi = int(pkt_match.group("rssi"))
                    current_att_experiment.add_rssi(rssi)
                    current_att_experiment.set_rx_count(current_att_experiment.get_rx_count() + 1)
                
                if (phy_match):
                    experiment_index += 1
                    break
        
        print("Results of experiment %d:\n" % (experiment_index))
        print("PHY of TX and RX:\t%s" % (current_att_experiment.get_phy()))
        print("TRX PRR:\t\t%.3f" % (current_att_experiment.packet_success()))
        print("Average RSSI:\t\t%.2f" % (current_att_experiment.get_rssi()))
        print("---------------------------------------------------")

        csv_filename = "./" + csv_filename
        if not os.path.exists(os.path.dirname(csv_filename)):
            append_write = "w"

            try:
                os.makedirs(os.path.dirname(csv_filename))
            except OSError as exc: # Guard against race condition
                if exc.errno != errno.EEXIST:
                    raise
        else:
            append_write = "a"

        with open(csv_filename, append_write, newline='') as output_file:
            output_file.write("%s,%.3f,%.2f\n" % (current_att_experiment.get_phy(),current_att_experiment.packet_success(),current_att_experiment.get_rssi()))