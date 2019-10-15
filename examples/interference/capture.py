#!/usr/bin/env python3

import sys
import datetime
import logging
import io
import argparse
import threading

# Parse commandline arguments

parser = argparse.ArgumentParser()
parser.add_argument("-f", "--filename", help="Specify the name of the logfile that will be created. The \".log\"-extension will be appended automatically. If a logfile with that name already exists, then the new information will be appended to the end of that file.")
parser.add_argument("-t", "--time", help="Time in seconds that the logging should take place from the start of the script in seconds. When that amount of time has passed, this script will exit.")
args = parser.parse_args()

# Set up logging

log = logging.getLogger('capture')
log.setLevel(logging.INFO)

handler_stream = logging.StreamHandler()
log.addHandler(handler_stream)

log_filename_without_extension = ""

if (args.filename):
    log_filename_without_extension = args.filename
else:
    log_filename_without_extension = datetime.datetime.now().strftime("log_%d-%m-%Y_%H-%M-%S-%f")

log_filename = "{}.log".format(log_filename_without_extension)

handler_file = logging.FileHandler(log_filename)
log.addHandler(handler_file)

log.info("Created logfile \"{}\"".format(log_filename))

# Capture stdin

start_time = datetime.datetime.now()
halt_event = threading.Event()

if (args.time):
    timer_thread = threading.Timer(int(args.time), halt_event.set)
    timer_thread.start()

for line in io.TextIOWrapper(sys.stdin.buffer, encoding='utf-8'):

    log.info("{}".format(line.rstrip('\r\n')))

    if (halt_event.is_set()):
        log.info("The logging stopped because the time of {} s has passed".format(args.time))
        sys.exit(0)
