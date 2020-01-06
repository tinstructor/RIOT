import shlex
import subprocess
import atexit
import logging
import io
import datetime
import threading

transmitter_phy = "SUN-OFDM 863-870MHz O3 MCS1"
interferer_phy = "SUN-OFDM 863-870MHz O4 MCS2"
sinr = 0 # in dB
test_duration = 150 # in seconds
offset_options = [0,1,2]
offset_values = [-2800,15840,31700]

# TODO for each given combination of 2 PHYs and a SINR, the following 2 steps are repeated for
# each of the 3 given offset options between the given PHYs:

    # First set the next offset option by sending the appropriate command over a serial connection
    # to the timing controller node.

    # Second capture the RX serial output of the interference test between the given PHYs in a logfile.
    # The name of the logfile can be anything and is preferrably removed after the next step (analysis).
    # Use a formatted timestamp for the names of the logfiles so that they're unique! Use the time
    # attribute of the capture script to stop the capturing after the given test duration and some
    # fixed buffer offset (just being safe).

    # Send the appropriate command to the timing controller which causes it to trigger signalling pins
    # and thus effectively start the experiment

    # When a test is over, the stream to the logfile is automatically closed after the duration
    # we passed to the capture script. However, since the controller script is unaware of the timing of
    # the seperately running capture script, a timer is needed to hold off the analyzer script
    # until we're sure that the capture script has stopped running. Alternatively we might be
    # able to check the status of the capture script through the subprocess library and then run
    # the analyzer script as soon as the capture script has exited.

    # When the timer runs out, the analyzer script can be started. The filenames can be constructed
    # from all information available in this script. Make sure to pass the append argument to the
    # analyzer script if the csv file with the constructed name is already present in the output
    # directory