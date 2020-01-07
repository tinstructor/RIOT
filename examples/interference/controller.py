import shlex
import subprocess
import atexit
import logging
import io
import datetime
import threading
import time
import os

transmitter_phy = "SUN-OFDM 863-870MHz O3 MCS1"
interferer_phy = "SUN-OFDM 863-870MHz O4 MCS2"
payload_size = 120 # in bytes
sinr = 0 # in dB
test_duration = 5 # in seconds
offset_values = [-2800,15840,31700]

halt_event = threading.Event()

def exit_handler():
    timing_shell.stdin.close()
    timing_shell.terminate()
    rx_shell.stdin.close()
    rx_shell.terminate()
    py_shell.stdin.close()
    py_shell.terminate()
    tx_shell.stdin.close()
    tx_shell.terminate()
    logging.shutdown()
    print("Exiting")

atexit.register(exit_handler)

for index, value in enumerate(offset_values):
    # First set the next offset option by sending the appropriate command over a serial connection
    # to the timing controller node.
    timing_shell = subprocess.Popen(shlex.split('make term PORT=/dev/ttyUSB4 BOARD=remote-revb -C /home/relsas/RIOT-benpicco/examples/timing_control/'),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True,bufsize=1)
    time.sleep(2)
    timing_shell.stdin.write("o%d\n" % (index))

    # Second capture the RX serial output of the interference test between the given PHYs in a logfile.
    # The name of the logfile can be anything and is preferrably removed after the next step (analysis).
    # Use a formatted timestamp for the names of the logfiles so that they're unique! Stop capturing
    # after a given test duration and some fixed buffering offset (just being safe).
    rx_shell = subprocess.Popen(shlex.split('make term PORT=/dev/ttyUSB1 BOARD=openmote-b'),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True,bufsize=1)
    time.sleep(2)
    rx_log = logging.getLogger('rx_logger')
    rx_log.setLevel(logging.INFO)
    rx_log_filename = "%s.log" % (datetime.datetime.now().strftime("rx_log_%d-%m-%Y_%H-%M-%S-%f"))
    rx_log.addHandler(logging.FileHandler(rx_log_filename))
    rx_log.info("Created logfile %s" % (rx_log_filename))
    threading.Timer(test_duration + 5, halt_event.set).start()

    # Send the appropriate command to the timing controller which causes it to trigger signalling pins
    # and thus effectively starts the experiment
    timing_shell.stdin.write("s\n")
    while True:
        # NOTE readline is blocking and if nothing is read this may cause you a headache
        rx_log.info("%s" % (rx_shell.stdout.readline().strip()))
        if halt_event.is_set():
            break

    # When a test is over, the stream to the logfile must be closed when the given test duration
    # has elapsed.
    logging.shutdown()

    # When the timer runs out, the analyzer script can be started. The filenames can be constructed
    # from all information available in this script. Make sure to pass the append argument to the
    # analyzer script if the csv file with the constructed name is already present in the output
    # directory
    csv_filename = "./TX_%dB_OF_%dUS_SIR_%dDB.csv" % (payload_size,value,sinr)
    analyzer_cmd = 'python3 analyzer.py %s %s -i \"%s\" -t \"%s\"' % (rx_log_filename,csv_filename,interferer_phy,transmitter_phy)
    if os.path.exists(os.path.dirname(csv_filename)):
        analyzer_cmd = analyzer_cmd + " -a"
    py_shell = subprocess.Popen(shlex.split(analyzer_cmd),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True,bufsize=0)

    # Reset all nodes before closing down the connections
    timing_shell.stdin.write("r\n")
    rx_shell.stdin.write("r\n")
    tx_shell = subprocess.Popen(shlex.split('make term PORT=/dev/ttyUSB3 BOARD=openmote-b'),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True,bufsize=1)
    time.sleep(2)
    tx_shell.stdin.write("r\n")

    timing_shell.stdin.close()
    timing_shell.terminate()
    rx_shell.stdin.close()
    rx_shell.terminate()
    py_shell.stdin.close()
    py_shell.terminate()
    tx_shell.stdin.close()
    tx_shell.terminate()
