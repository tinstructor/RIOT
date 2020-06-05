import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
from scipy import interpolate
from numpy import nan as NaN
import math
import random
from matplotlib import rc
import os

rc("font",**{"family":"sans-serif","weight":"regular","sans-serif":["Fira Sans"]})

extension = "pdf"
transparent_flag = True
trx_payload_sizes = [21,22,45,54,86,108,118,173,182,237,255,364] # in bytes

tx_raw = pd.DataFrame()
for trx_payload_size in trx_payload_sizes:
    filename = "TX_%dB_AT_39DB.csv" % trx_payload_size
    if os.path.isfile(filename):
        if tx_raw.empty:
            tx_raw = pd.read_csv(filename,header=None)
            tx_raw.columns = ["PHY","PRR","RSSI"]
            tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)
            tx_raw = tx_raw[tx_raw["PRR"] >= 0.05]
            tx_raw.insert(0,"PSDU",trx_payload_size)
        else:
            temp_df = pd.read_csv(filename,header=None)
            temp_df.columns = ["PHY","PRR","RSSI"]
            temp_df.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)
            temp_df = temp_df[temp_df["PRR"] >= 0.05]
            temp_df.insert(0,"PSDU",trx_payload_size)
            tx_raw = pd.concat([tx_raw,temp_df])
            tx_raw.reset_index(drop=True,inplace=True)

phy_list = sorted(tx_raw["PHY"].unique().tolist())
colors = ["C0","C1","C2","C3"]
tx_raw = tx_raw.pivot_table(index=["PSDU"],columns=["PHY"],values=["PRR"],fill_value=-1)
tx_raw.plot.bar(width=0.60,legend=False,color=colors,figsize=(7,4),zorder=3)

# plt.title("Lorem Ipsum",fontsize=14,fontweight="regular")
plt.gca().set_ylim(0,1.05)
plt.gca().set_ylabel("PRR")
plt.gca().yaxis.get_label().set_fontsize(13)
plt.gca().yaxis.get_label().set_weight("regular")
plt.gca().set_xlabel("PSDU [B]")
plt.gca().xaxis.get_label().set_fontsize(13)
plt.gca().xaxis.get_label().set_weight("regular")
plt.gca().xaxis.labelpad = 7
plt.gca().grid(which="both",axis="y",zorder=-1)
# plt.legend(phy_list,title="",loc="lower center",ncol=4)

rects = plt.gca().patches
for rect in rects:
    if rect.get_height() < 0:
        plt.gca().plot(rect.get_x() + rect.get_width() / 2, 0.03, marker='o', markersize=2.7, color="red", linestyle="None")

image_file = "baseline.%s" % extension
plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
plt.close()
