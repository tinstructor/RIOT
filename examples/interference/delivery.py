import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
from scipy import interpolate
from numpy import nan as NaN
import math
import random
from matplotlib import rc
import os.path

rc("font",**{"family":"sans-serif","weight":"regular","sans-serif":["Fira Sans"]})

def get_offset(phy_tuple,payload_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((payload_tuple[0]  * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    mid_trx_payload_offset = int(round(((trx_duration_us + pfhr_duration_us) / 2) - (if_duration_us / 2)))
    return mid_trx_payload_offset

# TODO return NaN if exceeds 100% and drop rows containing NaN before trying to pivot
# with the index set to the "Payload overlap" column (which won't work if NaN)
def get_payload_overlap(phy_tuple,payload_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    # NOTE payload_tuple(if_payload_size,trx_payload_size)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    if_sym = (math.ceil(((payload_tuple[0] * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym)
    trx_sym = (math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym)
    return (NaN if trx_sym - pfhr_sym < if_sym else if_sym / (trx_sym - pfhr_sym))

extension = "png"
transparent_flag = False
trx_payload_size = 120 # in bytes
if_payload_sizes = [20,25,30,35,40] # in bytes

tx_complete = pd.DataFrame()
for if_payload_size in if_payload_sizes:
    a = [2,3,4,5]
    b = [2,3,4,5]
    offset_list = [get_offset((if_idx,trx_idx),(if_payload_size,trx_payload_size)) for if_idx,trx_idx in [(x,y) for x in a for y in b]]
    offset_list = sorted(list(set(offset_list)))
    
    tx_raw = pd.DataFrame()
    for offset in offset_list:
        filename = "/home/relsas/RIOT-benpicco/examples/interference/IF_%dB_TX_%dB_OF_%dUS_SIR_0DB.csv"%(if_payload_size,trx_payload_size,offset)
        if os.path.isfile(filename):
            if tx_raw.empty:
                tx_raw = pd.read_csv(filename,header=None)
            else:
                tx_raw = pd.concat([tx_raw,pd.read_csv(filename,header=None)])

    if not tx_raw.empty:   
        tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","TRX PRR","IF PRR"]
        tx_raw.insert(0,"TX / RX payload",trx_payload_size)
        tx_raw.insert(0,"Interferer payload",if_payload_size)
        tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

        if tx_complete.empty:
            tx_complete = tx_raw
        else:
            tx_complete = pd.concat([tx_complete,tx_raw])
            
        tx_complete.reset_index(drop=True,inplace=True)

phy_names = {"O4 MCS2":2,"O4 MCS3":3,"O3 MCS1":4,"O3 MCS2":5}
tx_complete["Payload overlap"] = tx_complete.apply(lambda row: get_payload_overlap((phy_names[row["Interferer PHY\nconfiguration"]],phy_names[row["TX / RX PHY\nconfiguration"]]),(row["Interferer payload"],row["TX / RX payload"])),axis=1)
# tx_complete["Weighted TRX PRR"] = tx_complete["TRX PRR"] * tx_complete["Payload overlap"]

trx_phy_list = sorted(tx_complete["TX / RX PHY\nconfiguration"].unique().tolist())

fig, axes = plt.subplots(nrows=2, ncols=2,figsize=(16,9))
coord = {0:(0,0),1:(0,1),2:(1,0),3:(1,1)}
cmap = cm.get_cmap('Spectral')

for index, trx_phy in enumerate(trx_phy_list):
    trx_dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Payload overlap", columns="Interferer PHY\nconfiguration", values="TRX PRR")
    ax = trx_dfp.plot(ax=axes[coord[index][0],coord[index][1]],marker='x',markeredgewidth=1.8,markersize=8,linestyle="None",legend=False,colormap=cmap)

    for i,column_name in enumerate(list(trx_dfp.columns.values)):
        trx_x = trx_dfp[column_name].dropna().index.values
        trx_y = trx_dfp[column_name].dropna().to_numpy()
        trx_x_f = np.arange(trx_x[0],trx_x[-1]+0.001,0.001)
        trx_polynome = np.poly1d(np.polyfit(trx_x,trx_y,3))
        trx_y_f = trx_polynome(trx_x_f)
        axes[coord[index][0],coord[index][1]].plot(trx_x_f,trx_y_f,linewidth=1.5,color=ax.get_lines()[i].get_color())

    if_dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Payload overlap", columns="Interferer PHY\nconfiguration", values="IF PRR")
    ax = if_dfp.plot(ax=axes[coord[index][0],coord[index][1]],marker='x',markeredgewidth=1.8,markersize=8,linestyle="None",legend=False,colormap=cmap)

    for i,column_name in enumerate(list(if_dfp.columns.values)):
        if_x = if_dfp[column_name].dropna().index.values
        if_y = if_dfp[column_name].dropna().to_numpy()
        if_x_f = np.arange(if_x[0],if_x[-1]+0.001,0.001)
        if_polynome = np.poly1d(np.polyfit(if_x,if_y,3))
        if_y_f = if_polynome(if_x_f)
        axes[coord[index][0],coord[index][1]].plot(if_x_f,if_y_f,linewidth=1.5,color=ax.get_lines()[i].get_color())

    tick_offset = 0.02 if coord[index][1] < 1 else 0.04
    axes[coord[index][0],coord[index][1]].set_xticks(np.arange(round(trx_dfp.index.min(),2),round(trx_dfp.index.max(),2)+tick_offset,tick_offset).tolist())
    axes[coord[index][0],coord[index][1]].set_xlim(round(trx_dfp.index.min(),2)-0.01,round(trx_dfp.index.max(),2)+0.01)
    axes[coord[index][0],coord[index][1]].set_ylim(-0.05,1.05)
    axes[coord[index][0],coord[index][1]].set_ylabel("PRR")
    axes[coord[index][0],coord[index][1]].title.set_text("TX / RX PHY: " + trx_phy)
    axes[coord[index][0],coord[index][1]].title.set_fontsize(14)
    axes[coord[index][0],coord[index][1]].title.set_weight("regular")
    axes[coord[index][0],coord[index][1]].xaxis.get_label().set_fontsize(12)
    axes[coord[index][0],coord[index][1]].xaxis.get_label().set_weight("regular")
    axes[coord[index][0],coord[index][1]].yaxis.get_label().set_fontsize(12)
    axes[coord[index][0],coord[index][1]].yaxis.get_label().set_weight("regular")

handles, labels = axes[0,0].get_legend_handles_labels()
fig.legend(handles[0:4], labels[0:4], loc="lower center",ncol=4)

plt.subplots_adjust(wspace=0.15,hspace=0.35,top=0.9,left=0.1,right=0.9)
image_file = "IF_%d-%dB_TX_%dB.%s" % (if_payload_sizes[0],if_payload_sizes[-1],trx_payload_size,extension)
plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
plt.close()