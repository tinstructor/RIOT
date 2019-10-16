import io
import sys
import re
import argparse
import math

################################################################################

class experiment:
    def __init__(self, tx_count, trx_phy, if_phy):
        if (tx_count <= 0):
            raise ValueError("Amount of transmissions can't be negative or zero")
        self.tx_count = tx_count
        self.rx_count = 0
        self.trx_phy = trx_phy
        self.if_phy = if_phy

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

    def packet_success(self):
        if (self.rx_count == 0):
            return 0.0
        
        return self.rx_count / self.tx_count

    def packet_loss(self):
        return 1.0 - self.packet_success()

    def get_trx_phy(self):
        return self.trx_phy
    
    def get_if_phy(self):
        return self.if_phy

################################################################################

parser = argparse.ArgumentParser()
parser.add_argument("logfile", help="The logfile to be analyzed.")
args = parser.parse_args()

LOG_REGEXP_PKT = re.compile("^.*?PKT *?-")
LOG_REGEXP_PHY = re.compile("^.*?PHY")

NUM_OF_TX = 2
PHY_CONFIGS = ["SUN-FSK 863-870MHz OM1", "SUN-FSK 863-870MHz OM2",
               "SUN-OFDM 863-870MHz O4 MCS2", "SUN-OFDM 863-870MHz O4 MCS3",
               "SUN-OFDM 863-870MHz O3 MCS1", "SUN-OFDM 863-870MHz O3 MCS2"]

experiments = {}
trx_phy_index = 0
if_phy_index = 0
experiment_index = 0
current_experiment = experiment(NUM_OF_TX, PHY_CONFIGS[trx_phy_index], PHY_CONFIGS[if_phy_index])

log_filename = args.logfile

with open(log_filename, "r") as log:
    for line in log:

        chomped_line = line.rstrip()
        pkt_match = re.match(LOG_REGEXP_PKT, chomped_line)
        phy_match = re.match(LOG_REGEXP_PHY, chomped_line)

        if (pkt_match):
            current_experiment.set_rx_count(current_experiment.get_rx_count() + 1)
        elif (phy_match):
            if (trx_phy_index >= len(PHY_CONFIGS) - 1):
                trx_phy_index = 0
                if_phy_index += 1
            else:
                trx_phy_index += 1

            experiments[experiment_index] = current_experiment

            if (experiment_index < (len(PHY_CONFIGS) ** 2) - 1):
                current_experiment = experiment(NUM_OF_TX, PHY_CONFIGS[trx_phy_index], PHY_CONFIGS[if_phy_index])
                experiment_index += 1

for e in experiments:
    print("Results of experiment %d:\n" %(e))
    print("PHY of TX and RX:\t%s" % (experiments[e].get_trx_phy()))
    print("PHY of IF:\t\t%s" % (experiments[e].get_if_phy()))
    print("PRR:\t\t\t%.2f" % (experiments[e].packet_success()))
    print("---------------------------------------------------")
