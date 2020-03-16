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

extension = "png"
transparent_flag = False
pfhr_flag = True
trx_payload_size = 120 # in bytes
if_payload_sizes = [45] # in bytes

def get_offset_list(phy_tuple,payload_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    # NOTE payload_tuple(if_payload_size,trx_payload_size)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((payload_tuple[0] * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    if not pfhr_flag:
        mid_trx_payload_offset = int(round(((trx_duration_us + pfhr_duration_us) / 2) - (if_duration_us / 2)))
        return [mid_trx_payload_offset]
    else:
        trx_pfhr_offsets = [int(round(overlap_duration - if_duration_us)) for overlap_duration in ((np.arange(pfhr_sym) + 1) / ofdm_sym_rate) * 1000]
        return trx_pfhr_offsets

# TODO return NaN if exceeds 100% and drop rows containing NaN before trying to pivot
# with the index set to the "Payload overlap" column (which won't work if NaN)
def get_payload_overlap(phy_tuple,payload_tuple,offset):
    # NOTE phy_tuple(if_idx,trx_idx)
    # NOTE payload_tuple(if_payload_size,trx_payload_size)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((payload_tuple[0] * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym) / ofdm_sym_rate) * 1000
    if not pfhr_flag:
        return if_duration_us / (trx_duration_us - pfhr_duration_us)
    else:
        return (if_duration_us - abs(offset)) / pfhr_duration_us

tx_complete = pd.DataFrame()
for if_payload_size in if_payload_sizes:
    a = [2,3,4,5]
    b = [2,3,4,5]
    index_tuples = [(x,y) for x in a for y in b]
    if not pfhr_flag:
        offset_list = [get_offset_list((if_idx,trx_idx),(if_payload_size,trx_payload_size))[0] for if_idx,trx_idx in index_tuples]
    else:
        temp_list = [get_offset_list((if_idx,trx_idx),(if_payload_size,trx_payload_size)) for if_idx,trx_idx in index_tuples]
        offset_list = sum(temp_list,[])

    offset_list = sorted(list(set(offset_list)))
    
    tx_raw = pd.DataFrame()
    for offset in offset_list:
        if not pfhr_flag:
            filename = "/home/relsas/RIOT-benpicco/examples/interference/IF_%dB_TX_%dB_OF_%dUS_SIR_0DB.csv"%(if_payload_size,trx_payload_size,offset)
        else:
            filename = "/home/relsas/RIOT-benpicco/examples/interference/PFHR_IF_%dB_TX_%dB_OF_%dUS_SIR_0DB.csv"%(if_payload_size,trx_payload_size,offset)
        if os.path.isfile(filename):
            if tx_raw.empty:
                tx_raw = pd.read_csv(filename,header=None)
                tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","TRX PRR","IF PRR"]
                tx_raw.insert(0,"Offset",offset)
            else:
                temp_df = pd.read_csv(filename,header=None)
                temp_df.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","TRX PRR","IF PRR"]
                temp_df.insert(0,"Offset",offset)
                tx_raw = pd.concat([tx_raw,temp_df])

    if not tx_raw.empty:
        tx_raw.insert(0,"TX / RX payload",trx_payload_size)
        tx_raw.insert(0,"Interferer payload",if_payload_size)
        tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

        if tx_complete.empty:
            tx_complete = tx_raw
        else:
            tx_complete = pd.concat([tx_complete,tx_raw])
            
        tx_complete.reset_index(drop=True,inplace=True)

phy_names = {"O4 MCS2":2,"O4 MCS3":3,"O3 MCS1":4,"O3 MCS2":5}
tx_complete["Payload overlap"] = tx_complete.apply(lambda row: get_payload_overlap((phy_names[row["Interferer PHY\nconfiguration"]],phy_names[row["TX / RX PHY\nconfiguration"]]),(row["Interferer payload"],row["TX / RX payload"]),row["Offset"]),axis=1)
trx_phy_list = sorted(tx_complete["TX / RX PHY\nconfiguration"].unique().tolist())

fig, axes = plt.subplots(nrows=2, ncols=2,figsize=(16,9))
coord = {0:(0,0),1:(0,1),2:(1,0),3:(1,1)}

for index, trx_phy in enumerate(trx_phy_list):
    bottom_colors = ["C0","C1","C2","C3"]
    trx_dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].copy()
    trx_dfp = trx_dfp.pivot_table(index=["Payload overlap"],columns=["Interferer PHY\nconfiguration"],values=["TRX PRR"],fill_value=-1)
    trx_dfp.plot.bar(width=0.5,ax=axes[coord[index][0],coord[index][1]],legend=False,color=bottom_colors)

    rects = axes[coord[index][0],coord[index][1]].patches

    label_present = False
    for rect in rects:
        if rect.get_height() < 0:
            if not label_present:
                axes[coord[index][0],coord[index][1]].plot(rect.get_x() + rect.get_width() / 2, 0.05, marker='o', markersize=2.8, color="red", linestyle="None", label="N/A")
                label_present = True
            else:
                axes[coord[index][0],coord[index][1]].plot(rect.get_x() + rect.get_width() / 2, 0.05, marker='o', markersize=2.8, color="red", linestyle="None")

    top_colors = ["C4","C5","C6","C7"]
    if_dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].copy()
    if_dfp["IF PRR"] += if_dfp["TRX PRR"]
    if_dfp = if_dfp.pivot_table(index=["Payload overlap"],columns=["Interferer PHY\nconfiguration"],values=["IF PRR"])
    if_dfp.plot.bar(width=0.5,ax=axes[coord[index][0],coord[index][1]],legend=False,color=top_colors,zorder=-1)

    axes[coord[index][0],coord[index][1]].set_ylim(0,2.05)
    axes[coord[index][0],coord[index][1]].set_ylabel("Combined PRR")
    axes[coord[index][0],coord[index][1]].title.set_text("TX / RX PHY: " + trx_phy)
    axes[coord[index][0],coord[index][1]].title.set_fontsize(14)
    axes[coord[index][0],coord[index][1]].title.set_weight("regular")
    axes[coord[index][0],coord[index][1]].xaxis.get_label().set_fontsize(12)
    axes[coord[index][0],coord[index][1]].xaxis.get_label().set_weight("regular")
    axes[coord[index][0],coord[index][1]].xaxis.labelpad = 8
    axes[coord[index][0],coord[index][1]].yaxis.get_label().set_fontsize(12)
    axes[coord[index][0],coord[index][1]].yaxis.get_label().set_weight("regular")

    labels = [item.get_text() for item in axes[coord[index][0],coord[index][1]].get_xticklabels()]
    axes[coord[index][0],coord[index][1]].set_xticklabels(["%.3f" % round(float(label),3) for label in labels])

handles, labels = axes[0,0].get_legend_handles_labels()
if not pfhr_flag:
    handles.insert(len(handles) - 1,handles.pop(0))
    labels.insert(len(labels) - 1,labels.pop(0))
    fig.legend(handles, labels, loc="lower center",ncol=5)
else:
    fig.legend(handles, labels, loc="lower center",ncol=4)

plt.subplots_adjust(wspace=0.14,hspace=0.42,top=0.9,left=0.1,right=0.9,bottom=0.16)
if not pfhr_flag:
    image_file = "IF_%d-%dB_TX_%dB.%s" % (if_payload_sizes[0],if_payload_sizes[-1],trx_payload_size,extension)
else:
    image_file = "PFHR_IF_%d-%dB_TX_%dB.%s" % (if_payload_sizes[0],if_payload_sizes[-1],trx_payload_size,extension)
plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
plt.close()