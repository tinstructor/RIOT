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

ON_POSIX = 'posix' in sys.builtin_module_names

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
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (3,"SUN-OFDM 863-870MHz O4 MCS3"), 
               (6,"SUN-OFDM 863-870MHz O4 MCS4"), (7,"SUN-OFDM 863-870MHz O4 MCS5")]
trx_payload_sizes = [25,50,75,100,125] # in bytes
trx_dest_addr = "22:68:31:23:9D:F1:96:37"
attenuation = 0 # in dB
num_of_tx = 200
test_duration = int(round(0.5 * num_of_tx)) + 2 # in seconds

halt_event = threading.Event()

# NOTE you might have to change the serial port numbers of the devices
# depending on the order in which you plugged them into the USB ports
timing_cmd = "make term PORT=/dev/ttyUSB6 BOARD=remote-revb -C /home/relsas/RIOT-benpicco/examples/timing_control/"
rx_cmd = "make term PORT=/dev/ttyUSB1 BOARD=openmote-b"
tx_cmd = "make term PORT=/dev/ttyUSB3 BOARD=openmote-b"

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

        csv_filename = "./TX_%dB_AT_%dDBM.csv" % (trx_payload_size,attenuation)
        # TODO write a new script like analyzer.py