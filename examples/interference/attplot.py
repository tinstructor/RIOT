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
trx_payload_sizes = [127] # in bytes

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
    tx_raw = tx_raw[tx_raw["PRR"] >= 0.05]
    tx_raw.drop_duplicates(inplace=True)
    tx_raw.reset_index(drop=True,inplace=True)

    phy_list = tx_raw["PHY"].unique()
    for index,phy in enumerate(phy_list):
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-103.50]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-103.00]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-102.00]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-101.00]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-100.00]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-99.00]
        tx_raw.loc[len(tx_raw)] = [phy,1.000,-98.00]
        # print(tx_raw)
        colors = ["C0","C1","C2","C3","C4","C5","C6"]
        sns.regplot(x="RSSI",y="PRR",data=tx_raw[tx_raw["PHY"] == phy],order=7,fit_reg=True,truncate=False,ci=None,color=colors[index])
    plt.title("PRR in function of average\nRSSI for %dB PSDU" % trx_payload_size,fontsize=14,fontweight="regular")
    plt.gca().set_ylim(-0.05,1.05)
    plt.gca().yaxis.get_label().set_fontsize(12)
    plt.gca().yaxis.get_label().set_weight("regular")
    plt.gca().set_xlim(-116.90,-103.85)
    plt.gca().xaxis.get_label().set_fontsize(12)
    plt.gca().xaxis.get_label().set_weight("regular")
    plt.legend(phy_list,title="")

    image_file = "TX_%dB.%s" % (trx_payload_size,extension)
    plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
    plt.close()