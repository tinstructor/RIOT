import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib import rc
import math
import os
import fnmatch

rc("font",**{"family":"sans-serif","weight":"regular","sans-serif":["Fira Sans"]})

extension = "png"
transparent_flag = False
trx_payload_sizes = [20,70,120] # in bytes

for trx_payload_size in trx_payload_sizes:
    csv_list = fnmatch.filter(os.listdir("."), "TX_%dB_AT*.csv" % trx_payload_size)
    tx_raw = pd.DataFrame()
    for filename in csv_list:
        if tx_raw.empty:
            tx_raw = pd.read_csv(filename,header=None)
        else:
            tx_raw = pd.concat([tx_raw,pd.read_csv(filename,header=None)])
    tx_raw.columns = ["PHY","PRR","RSSI"]
    tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)
    tx_raw = tx_raw[tx_raw["PRR"] > 0.05]
    tx_raw.drop_duplicates(inplace=True)
    tx_raw.reset_index(drop=True,inplace=True)
    tx_matrix = tx_raw.pivot_table(index=["RSSI"],columns=["PHY"],values=["PRR"])
    ax = tx_matrix.plot(linestyle="None",marker="x")
    ax.set_ylabel("PRR")
    ax.yaxis.get_label().set_fontsize(12)
    ax.yaxis.get_label().set_weight("regular")
    ax.set_xlabel("RSSI")
    ax.xaxis.get_label().set_fontsize(12)
    ax.xaxis.get_label().set_weight("regular")
    ax.legend(title="")
    ax.title.set_text("PRR in function of average\nRSSI for %dB PSDU" % trx_payload_size)
    ax.title.set_fontsize(14)
    ax.title.set_weight("regular")
    image_file = "TX_%dB.%s" % (trx_payload_size,extension)
    plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
    plt.close()