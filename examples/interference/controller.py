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
import numpy as np
from scipy import interpolate

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
    if isinstance(if_shell, subprocess.Popen):
        if_shell.kill()
    print("Exiting")

atexit.register(exit_handler)

# NOTE changes the following set of values before starting the script in order
# to reflect the correct scenario
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1")]#,
               #(3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
if_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1")]#,
              #(3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
trx_payload_size = 120 # in bytes
if_payload_size = 21 # in bytes
sinr = 0 # in dB
num_of_tx = 5
test_duration = int(round(0.5 * num_of_tx)) + 2 # in seconds

# NOTE offset compensation is calculated by means of 2D interpolation. You can get 
# the appropriate compensation by calling rbf(if_payload_size,trx_payload_size)
# TODO while a negative compensation is possible and calling rbf might well give
# back a negative value, it is not currently possible to instruct the timing controller
# to make use of a negative offset compensation.
x = np.ogrid[20:130:10]
x = np.tile(x,11)
y = np.ogrid[20:130:10]
y = np.repeat(y,11)
z = np.array([20,-28,-70,-118,-154,-208,-256,-298,-352,-388,-442,
              62,20,-28,-70,-118,-154,-208,-256,-298,-352,-388,
              104,62,20,-28,-70,-118,-154,-208,-256,-298,-352,
              152,104,62,20,-28,-70,-118,-154,-208,-256,-298,
              194,152,104,62,20,-28,-70,-118,-154,-208,-256,
              242,194,152,104,62,20,-28,-70,-118,-154,-208,
              284,242,194,152,104,62,20,-28,-70,-118,-154,
              332,284,242,194,152,104,62,20,-28,-70,-118,
              380,332,284,242,194,152,104,62,20,-28,-70,
              428,380,332,284,242,194,152,104,62,20,-28,
              476,428,380,332,284,242,194,152,104,62,20])
rbf = interpolate.Rbf(x,y,z)
compensation = int(round(rbf(if_payload_size,trx_payload_size).tolist()))

# TODO calculate offsets based on payload sizes and combination of PHYs
def get_offset_list(phy_tuple):
    if (phy_tuple == (2,2) or phy_tuple == (2,4) or phy_tuple == (4,2) or phy_tuple == (4,4)):
        return [-3480,7920,16320]
    elif (phy_tuple == (3,2) or phy_tuple == (5,2) or phy_tuple == (3,4) or phy_tuple == (5,4)):
        return [-1800,8760,18000]
    elif (phy_tuple == (3,3) or phy_tuple == (5,3) or phy_tuple == (3,5) or phy_tuple == (5,5)):
        return [-1800,3960,8400]
    elif (phy_tuple == (2,3) or phy_tuple == (4,3) or phy_tuple == (2,5) or phy_tuple == (4,5)):
        return [-3480,3120,6720]
    return None

halt_event = threading.Event()

# NOTE you might have to change the serial port numbers of the devices
# depending on the order in which you plugged them into the USB ports
timing_cmd = "make term PORT=/dev/ttyUSB6 BOARD=remote-revb -C /home/relsas/RIOT-benpicco/examples/timing_control/"
rx_cmd = "make term PORT=/dev/ttyUSB1 BOARD=openmote-b"
tx_cmd = "make term PORT=/dev/ttyUSB3 BOARD=openmote-b"
if_cmd = "make term PORT=/dev/ttyUSB5 BOARD=openmote-b"

rx_q = Queue()

for if_idx, if_phy in if_phy_cfg:
    for trx_idx, trx_phy in trx_phy_cfg:
        for offset in get_offset_list((if_idx,trx_idx)):
            threading.Timer(1, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break

            timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
            try:
                timing_shell.communicate(input="ifphy %s\n" % (if_idx),timeout=2)
            except TimeoutExpired:
                timing_shell.kill()
            
            threading.Timer(2, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break

            timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
            try:
                timing_shell.communicate(input="trxphy %s\n" % (trx_idx),timeout=2)
            except TimeoutExpired:
                timing_shell.kill()

            threading.Timer(2, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break
            
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
                timing_shell.communicate(input="offset %s\n" % (offset),timeout=2)
            except TimeoutExpired:
                timing_shell.kill()

            threading.Timer(1, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break
            
            timing_shell = subprocess.Popen(shlex.split(timing_cmd),stdin=PIPE,universal_newlines=True)
            try:
                timing_shell.communicate(input="comp %s\n" % (compensation),timeout=2)
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

            rx_shell = subprocess.Popen(shlex.split(rx_cmd),stdin=PIPE,stdout=PIPE,stderr=PIPE,universal_newlines=True,bufsize=1,close_fds=ON_POSIX)

            rx_q.clear()
            rx_t = Thread(target=enqueue_output, args=(rx_shell.stdout, rx_q))
            rx_t.daemon = True
            rx_t.start()
            
            rx_log_filename = "%s.log" % (datetime.datetime.now().strftime("rx_log_%d-%m-%Y_%H-%M-%S-%f"))
            rx_logfile = open(rx_log_filename,"w",newline='')
            rx_logfile.write("Created logfile %s\n" % (rx_log_filename))

            threading.Timer(test_duration + 3, halt_event.set).start()
            while True:
                try:  
                    line = rx_q.get_nowait()
                except queue.Empty:
                    if halt_event.is_set():
                        halt_event.clear()
                        break
                else: # got line
                    if line == '' and rx_shell.poll() is not None:
                        break
                    if line != '':
                        print("%s" % (line.strip().strip("\r\n")))
                        rx_logfile.write("%s\n" % (line.strip().strip("\r\n")))

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

            threading.Timer(1, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break

            if_shell = subprocess.Popen(shlex.split(if_cmd),stdin=PIPE,universal_newlines=True)
            try:
                if_shell.communicate(input="reboot\n",timeout=2)
            except TimeoutExpired:
                if_shell.kill()
            
            threading.Timer(1, halt_event.set).start()
            while True:
                if halt_event.is_set():
                    halt_event.clear()
                    break

            try:
                rx_shell.communicate(input="reboot\n",timeout=2)
            except TimeoutExpired:
                rx_shell.kill()
            
            csv_filename = "./TX_%dB_OF_%dUS_SIR_%dDB.csv" % (trx_payload_size,offset,sinr)
            analyzer_cmd = "python3 analyzer.py %s %s -i \"%s\" -t \"%s\" -n %d" % (rx_log_filename,csv_filename,if_phy,trx_phy,num_of_tx)
            if os.path.exists(os.path.dirname(csv_filename)):
                analyzer_cmd = analyzer_cmd + " -a"
            py_shell = subprocess.Popen(shlex.split(analyzer_cmd),stdout=PIPE,stderr=PIPE,universal_newlines=True)
            try:
                outs, errs = py_shell.communicate(timeout=30)
            except TimeoutExpired:
                py_shell.kill()
                outs, errs = py_shell.communicate()
            print(outs)
