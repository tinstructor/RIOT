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
import math

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
    if isinstance(ift_shell, subprocess.Popen):
        ift_shell.kill()
    if isinstance(ifr_shell, subprocess.Popen):
        ifr_shell.kill()
    print("Exiting")

atexit.register(exit_handler)

# NOTE changes the following set of values before starting the script in order
# to reflect the correct scenario
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
               (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
if_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
              (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
trx_payload_size = 120 # in bytes
if_payload_sizes = [50,70,90] # in bytes
trx_dest_addr = "22:68:31:23:9D:F1:96:37"
if_dest_addr = "22:68:31:23:14:F1:99:37"
sinr = 0 # in dB
num_of_tx = 4
test_duration = int(round(0.5 * num_of_tx)) + 2 # in seconds

# NOTE offset compensation is calculated by means of 2D interpolation. You can get 
# the appropriate compensation by calling rbf(if_payload_size,trx_payload_size)
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

def get_compensation(size_tuple):
    # NOTE size_tuple = (if_payload_size,trx_payload_size)
    compensation = int(round(rbf(size_tuple[0],size_tuple[1]).tolist()))
    return compensation

# TODO make this function compatible with FSK again
# TODO also return a list for offsets in PFHR (if flag is set)
def get_offset_list(phy_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((if_payload_size * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((trx_payload_size * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    mid_trx_payload_offset = int(round(((trx_duration_us + pfhr_duration_us) / 2) - (if_duration_us / 2)))
    return [mid_trx_payload_offset]

def get_payload_overlap(phy_tuple,payload_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    # NOTE payload_tuple(if_payload_size,trx_payload_size)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    if_sym = (math.ceil(((payload_tuple[0] * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym)
    trx_sym = (math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym)
    return if_sym / (trx_sym - pfhr_sym)

halt_event = threading.Event()

# NOTE you might have to change the serial port numbers of the devices
# depending on the order in which you plugged them into the USB ports
timing_cmd = "make term PORT=/dev/ttyUSB6 BOARD=remote-revb -C /home/relsas/RIOT-benpicco/examples/timing_control/"
rx_cmd = "make term PORT=/dev/ttyUSB1 BOARD=openmote-b"
tx_cmd = "make term PORT=/dev/ttyUSB3 BOARD=openmote-b"
ift_cmd = "make term PORT=/dev/ttyUSB5 BOARD=openmote-b"
ifr_cmd = "make term PORT=/dev/ttyUSB8 BOARD=openmote-b"

rx_q = Queue()
ifr_q = Queue()

# TODO check if combination of payload sizes and PHYs would exceed 100% overlap
# and don't execute experiment if that's the case. Make sure to leave opening 
# for future versions where not all overlap is strictly in the payload.
for if_payload_size in if_payload_sizes:
    for if_idx, if_phy in if_phy_cfg:
        for trx_idx, trx_phy in trx_phy_cfg:
            if get_payload_overlap((if_idx,trx_idx),(if_payload_size,trx_payload_size)) <= 1.0:
                for offset in get_offset_list((if_idx,trx_idx)):
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
                    # commands influence the configuration of 2 seperate nodes (receiver & ifr)

                    ifr_shell = subprocess.Popen(shlex.split(ifr_cmd),stdin=PIPE,universal_newlines=True)
                    try:
                        ifr_shell.communicate(input="physub %s\n" % (if_idx),timeout=2)
                    except TimeoutExpired:
                        ifr_shell.kill()

                    # NOTE an additional waiting period is not needed here because these 2 consecutive shell
                    # commands influence the configuration of 2 seperate nodes (ifr & ift)
                    
                    ift_shell = subprocess.Popen(shlex.split(ift_cmd),stdin=PIPE,universal_newlines=True)
                    try:
                        ift_shell.communicate(input="numbytesub %s\n" % (if_payload_size - 19),timeout=2)
                    except TimeoutExpired:
                        ift_shell.kill()

                    threading.Timer(1, halt_event.set).start()
                    while True:
                        if halt_event.is_set():
                            halt_event.clear()
                            break
                    
                    ift_shell = subprocess.Popen(shlex.split(ift_cmd),stdin=PIPE,universal_newlines=True)
                    try:
                        ift_shell.communicate(input="saddrsub %s\n" % (if_dest_addr),timeout=2)
                    except TimeoutExpired:
                        ift_shell.kill()

                    threading.Timer(1, halt_event.set).start()
                    while True:
                        if halt_event.is_set():
                            halt_event.clear()
                            break
                    
                    ift_shell = subprocess.Popen(shlex.split(ift_cmd),stdin=PIPE,universal_newlines=True)
                    try:
                        ift_shell.communicate(input="physub %s\n" % (if_idx),timeout=2)
                    except TimeoutExpired:
                        ift_shell.kill()

                    # NOTE an additional waiting period is not needed here because these 2 consecutive shell
                    # commands influence the configuration of 2 seperate nodes (interferer & timing controller)
                    
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
                        timing_shell.communicate(input="comp %s\n" % (get_compensation((if_payload_size,trx_payload_size))),timeout=2)
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

                    ifr_shell = subprocess.Popen(shlex.split(ifr_cmd),stdin=PIPE,stdout=PIPE,stderr=PIPE,universal_newlines=True,bufsize=1,close_fds=ON_POSIX)

                    ifr_q.clear()
                    ifr_t = Thread(target=enqueue_output, args=(ifr_shell.stdout, ifr_q))
                    ifr_t.daemon = True
                    ifr_t.start()
                    
                    created_on = datetime.datetime.now().strftime("%d-%m-%Y_%H-%M-%S-%f")

                    rx_log_filename = "rx_log_%s.log" % created_on
                    rx_logfile = open(rx_log_filename,"w",newline='')
                    rx_logfile.write("Created logfile %s\n" % (rx_log_filename))

                    ifr_log_filename = "ifr_log_%s.log" % created_on
                    ifr_logfile = open(ifr_log_filename,"w",newline='')
                    ifr_logfile.write("Created logfile %s\n" % (ifr_log_filename))

                    threading.Timer(test_duration + 3, halt_event.set).start()
                    while True:
                        try:  
                            rx_line = rx_q.get_nowait()
                            ifr_line = ifr_q.get_nowait()
                        except queue.Empty:
                            if halt_event.is_set():
                                halt_event.clear()
                                break
                        else: # got line
                            if rx_line == '' and rx_shell.poll() is not None:
                                break
                            if ifr_line == '' and ifr_shell.poll() is not None:
                                break
                            if rx_line != '':
                                # print("%s" % (rx_line.strip().strip("\r\n")))
                                rx_logfile.write("%s\n" % (rx_line.strip().strip("\r\n")))
                            if ifr_line != '':
                                # print("%s" % (ifr_line.strip().strip("\r\n")))
                                ifr_logfile.write("%s\n" % (ifr_line.strip().strip("\r\n")))
                    
                    rx_logfile.write("PHY\n")
                    rx_logfile.close()
                    ifr_logfile.write("NEXT_EXP\n")
                    ifr_logfile.write("PHY\n")
                    ifr_logfile.close()

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

                    ift_shell = subprocess.Popen(shlex.split(ift_cmd),stdin=PIPE,universal_newlines=True)
                    try:
                        ift_shell.communicate(input="reboot\n",timeout=2)
                    except TimeoutExpired:
                        ift_shell.kill()
        
                    try:
                        rx_shell.communicate(input="reboot\n",timeout=2)
                    except TimeoutExpired:
                        rx_shell.kill()

                    try:
                        ifr_shell.communicate(input="reboot\n",timeout=2)
                    except TimeoutExpired:
                        ifr_shell.kill()
                    
                    csv_filename = "./IF_%dB_TX_%dB_OF_%dUS_SIR_%dDB.csv" % (if_payload_size,trx_payload_size,offset,sinr)
                    analyzer_cmd = "python3 analyzer.py %s %s -i \"%s\" -t \"%s\" -n %d -l %s" % (rx_log_filename,csv_filename,if_phy,trx_phy,num_of_tx,ifr_log_filename)
                    if os.path.exists(os.path.dirname(csv_filename)):
                        analyzer_cmd = analyzer_cmd + " -a"
                    py_shell = subprocess.Popen(shlex.split(analyzer_cmd),stdout=PIPE,stderr=PIPE,universal_newlines=True)
                    try:
                        outs, errs = py_shell.communicate(timeout=30)
                    except TimeoutExpired:
                        py_shell.kill()
                        outs, errs = py_shell.communicate()
                    print(outs)
