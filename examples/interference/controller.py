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

# NOTE change the following set of values before starting the script in order
# to reflect the correct scenario
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
               (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
if_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
              (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
trx_payload_size = 255 # in bytes
trx_dest_addr = "22:68:31:23:9D:F1:96:37"
if_dest_addr = "22:68:31:23:14:F1:99:37"
sir = 0 # in dB
num_of_tx = 10
pfhr_flag = True
test_duration = int(round(0.5 * num_of_tx)) + 2 # in seconds

# NOTE offset compensation is calculated by means of 2D interpolation. You can get 
# the appropriate compensation by calling rbf(if_payload_size,trx_payload_size)
# TODO add values for x (= if_payload_size) going to 370 and y (= trx_payload_size)
# going to 260 so that the interpolation is accurate for all selected payload sizes.
# However it may be faster to fix trx_payload_size to 255 and just go with 1D inter-
# polation altogether
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

def get_compensation(if_pls,trx_pls):
    compensation = int(round(rbf(if_pls,trx_pls).tolist()))
    return compensation

def get_if_payload_sizes(if_idx,trx_idx):
    if not pfhr_flag:
        if (trx_idx == 2 or trx_idx == 4) and (if_idx == 2 or if_idx == 4):
            return [21,54,118,182]
        elif (trx_idx == 2 or trx_idx == 4) and (if_idx == 3 or if_idx == 5):
            return [21,108,237,364]
        elif (trx_idx == 3 or trx_idx == 5) and (if_idx == 2 or if_idx == 4):
            return [21,22,54,86]
        elif (trx_idx == 3 or trx_idx == 5) and (if_idx == 3 or if_idx == 5):
            return [21,45,108,173]
        else:
            raise ValueError("Index combination doesn't exist!")
    else:
        return [255]

def get_offsets(if_idx,trx_idx,if_pls,trx_pls):
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((if_pls * 8) + tail_bits) / udbps[if_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((trx_pls * 8) + tail_bits) / udbps[trx_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    if not pfhr_flag:
        mid_trx_payload_offset = int(round(((trx_duration_us + pfhr_duration_us) / 2) - (if_duration_us / 2)))
        return [mid_trx_payload_offset]
    else:
        trx_pfhr_offsets = [int(round(overlap_duration - if_duration_us)) for overlap_duration in ([4,6,12] / ofdm_sym_rate) * 1000]
        return trx_pfhr_offsets

halt_event = threading.Event()

# NOTE you might have to change the serial port numbers of the devices
# depending on the order in which you plugged them into the USB ports
timing_cmd = "make term PORT=/dev/ttyUSB8 BOARD=remote-revb -C /home/relsas/RIOT-benpicco/examples/timing_control/"
rx_cmd = "make term PORT=/dev/ttyUSB1 BOARD=openmote-b"
tx_cmd = "make term PORT=/dev/ttyUSB3 BOARD=openmote-b"
ift_cmd = "make term PORT=/dev/ttyUSB5 BOARD=openmote-b"
ifr_cmd = "make term PORT=/dev/ttyUSB7 BOARD=openmote-b"

for if_idx, if_phy in if_phy_cfg:
    for trx_idx, trx_phy in trx_phy_cfg:
        for if_payload_size in get_if_payload_sizes(if_idx,trx_idx):
            for offset in get_offsets(if_idx,trx_idx,if_payload_size,trx_payload_size):
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
                    timing_shell.communicate(input="comp %s\n" % (get_compensation(if_payload_size,trx_payload_size)),timeout=2)
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

                ifr_log_filename = "ifr_log_%s.log" % datetime.datetime.now().strftime("%d-%m-%Y_%H-%M-%S-%f")
                ifr_logfile = open(ifr_log_filename,"w",newline='')
                ifr_logfile.write("Created logfile %s\n" % (ifr_log_filename))

                rx_shell = subprocess.Popen(shlex.split(rx_cmd),stdin=PIPE,stdout=PIPE,stderr=PIPE,universal_newlines=True,bufsize=1,close_fds=ON_POSIX)

                rx_q = Queue()
                rx_t = Thread(target=enqueue_output, args=(rx_shell.stdout, rx_q))
                rx_t.daemon = True
                rx_t.start()

                ifr_shell = subprocess.Popen(shlex.split(ifr_cmd),stdin=PIPE,stdout=PIPE,stderr=PIPE,universal_newlines=True,bufsize=1,close_fds=ON_POSIX)

                ifr_q = Queue()
                ifr_t = Thread(target=enqueue_output, args=(ifr_shell.stdout, ifr_q))
                ifr_t.daemon = True
                ifr_t.start()

                threading.Timer(test_duration + 3, halt_event.set).start()
                while True:
                    try:
                        ifr_line = ifr_q.get_nowait()
                    except queue.Empty:
                        if halt_event.is_set():
                            halt_event.clear()
                            break
                    else:
                        # NOTE breaks when reading an empty line and the subprocess has terminated
                        if ifr_line == '' and ifr_shell.poll() is not None:
                            break
                        if ifr_line != '':
                            # print("%s" % (ifr_line.strip().strip("\r\n")))
                            ifr_logfile.write("%s\n" % (ifr_line.strip().strip("\r\n")))

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
                            # print("%s" % (rx_line.strip().strip("\r\n")))
                            rx_logfile.write("%s\n" % (rx_line.strip().strip("\r\n")))
                
                if halt_event.is_set():
                    halt_event.clear()
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
                
                if not pfhr_flag:
                    csv_filename = "./IF_%dB_TX_%dB_OF_%dUS_SIR_%dDB.csv" % (if_payload_size,trx_payload_size,offset,sir)
                else:
                    csv_filename = "./PFHR_IF_%dB_TX_%dB_OF_%dUS_SIR_%dDB.csv" % (if_payload_size,trx_payload_size,offset,sir)
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
