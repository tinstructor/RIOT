import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import math
from matplotlib import rc
import os.path
import datetime
from itertools import groupby

rc("font",**{"family":"sans-serif","weight":"regular","sans-serif":["Fira Sans"]})

extension = "png"
transparent_flag = False
pfhr_flag = False
trx_payload_size = 255 # in bytes
if_payload_sizes = [21,22,45,54,86,108,118,173,182,237,364] if not pfhr_flag else [255] # in bytes
sirs = [-3,0,3,6,9] # in dB

def get_offsets(if_idx,trx_idx,if_pls,trx_pls):
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((if_pls * 8) + tail_bits) / udbps[if_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((trx_pls * 8) + tail_bits) / udbps[trx_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    if not pfhr_flag:
        mid_trx_payload_offset = int(round((trx_duration_us + pfhr_duration_us - if_duration_us) / 2))
        return [mid_trx_payload_offset]
    else:
        overlap_durations_us = [(overlapping_symbols / ofdm_sym_rate) * 1000 for overlapping_symbols in [4,6,12]]
        trx_pfhr_offsets = [int(round(overlap_duration_us - if_duration_us)) for overlap_duration_us in overlap_durations_us]
        return trx_pfhr_offsets

def get_payload_overlap(if_idx,trx_idx,if_pls,trx_pls,offset):
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    ofdm_sym_rate = (8 + (1 / 3))
    pfhr_duration_us = (pfhr_sym / ofdm_sym_rate) * 1000
    if_duration_us = ((math.ceil(((if_pls * 8) + tail_bits) / udbps[if_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    trx_duration_us = ((math.ceil(((trx_pls * 8) + tail_bits) / udbps[trx_idx]) + pfhr_sym) / ofdm_sym_rate) * 1000
    if not pfhr_flag:
        return round(if_duration_us * 100 / (trx_duration_us - pfhr_duration_us),1)
    else:
        return round((if_duration_us - abs(offset)) * 100 / pfhr_duration_us,1)

tx_complete = pd.DataFrame()
for sir in sirs:
    for if_payload_size in if_payload_sizes:
        a = [2,3,4,5]
        b = [2,3,4,5]
        index_tuples = [(x,y) for x in a for y in b]
        if not pfhr_flag:
            offset_list = [get_offsets(if_idx,trx_idx,if_payload_size,trx_payload_size)[0] for if_idx,trx_idx in index_tuples]
        else:
            temp_list = [get_offsets(if_idx,trx_idx,if_payload_size,trx_payload_size) for if_idx,trx_idx in index_tuples]
            offset_list = sum(temp_list,[])

        offset_list = sorted(list(set(offset_list)))
        
        tx_raw = pd.DataFrame()
        for offset in offset_list:
            if not pfhr_flag:
                filename = "/home/relsas/RIOT-benpicco/examples/interference/PP_IF_%dB_TX_%dB_OF_%dUS_SIR_%dDB.csv"%(if_payload_size,trx_payload_size,offset,sir)
            else:
                filename = "/home/relsas/RIOT-benpicco/examples/interference/PFHR_IF_%dB_TX_%dB_OF_%dUS_SIR_%dDB.csv"%(if_payload_size,trx_payload_size,offset,sir)
            if os.path.isfile(filename):
                if tx_raw.empty:
                    tx_raw = pd.read_csv(filename,header=None)
                    tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","TRX PRR","IF PRR"]
                    tx_raw.insert(0,"Offset",offset)
                    tx_raw.insert(0,"Sir",sir)
                else:
                    temp_df = pd.read_csv(filename,header=None)
                    temp_df.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","TRX PRR","IF PRR"]
                    temp_df.insert(0,"Offset",offset)
                    temp_df.insert(0,"Sir",sir)
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
tx_complete["Payload overlap"] = tx_complete.apply(lambda row: get_payload_overlap(phy_names[row["Interferer PHY\nconfiguration"]],phy_names[row["TX / RX PHY\nconfiguration"]],row["Interferer payload"],row["TX / RX payload"],row["Offset"]),axis=1)
TRX_PRR = pd.pivot_table(tx_complete,index=["TX / RX PHY\nconfiguration","Payload overlap"],values=["TRX PRR"],columns=["Interferer PHY\nconfiguration","Sir"])
IF_PRR = pd.pivot_table(tx_complete,index=["TX / RX PHY\nconfiguration","Payload overlap"],values=["IF PRR"],columns=["Interferer PHY\nconfiguration","Sir"])

def add_h_line(ax,xpos,ypos):
    line = plt.Line2D([ypos,ypos+.08],[xpos, xpos],color='black',transform=ax.transAxes)
    line.set_clip_on(False)
    ax.add_line(line)

def add_v_line(ax,xpos,ypos):
    line = plt.Line2D([xpos,xpos],[ypos,ypos+(.05 if not pfhr_flag else .08)],color='black',transform=ax.transAxes)
    line.set_clip_on(False)
    ax.add_line(line)

def y_label_pos(my_index,level):
    labels = my_index.get_level_values(level)
    return [(k,sum(1 for i in g)) for k,g in groupby(labels)]

def x_label_pos(my_index,level):
    labels = my_index.get_level_values(level)
    return [(k,sum(1 for i in g)) for k,g in groupby(labels)]

def y_label_group(ax,df):
    xpos = -.08
    scale = 1./df.index.size
    for level in range(df.index.nlevels)[::-1]:
        pos = df.index.size
        for label, rpos in y_label_pos(df.index,level):
            add_h_line(ax, pos*scale, xpos)
            pos -= rpos
            lypos = (pos + .5 * rpos)*scale
            ax.text(xpos+.04, lypos, label, va='center', ha='center', transform=ax.transAxes, rotation = 0 if level == 1 else 90, fontsize=11,fontweight="regular") 
        add_h_line(ax, pos*scale , xpos)
        xpos -= .08

def x_label_group(ax, df):
    ypos = (-.05 if not pfhr_flag else -0.08)
    scale = 1./df.columns.size
    for level in range(df.columns.nlevels)[:0:-1]:
        pos = df.columns.size
        for label, rpos in y_label_pos(df.columns,level)[::-1]:
            add_v_line(ax, pos*scale, ypos)
            pos -= rpos
            lxpos = (pos + .5 * rpos)*scale
            ax.text(lxpos, ypos+(.025 if not pfhr_flag else .04), label, va='center', ha='center', transform=ax.transAxes, fontsize=11, fontweight="regular")
        add_v_line(ax, pos*scale , ypos)
        ypos -= (.05 if not pfhr_flag else 0.08)

fig = plt.figure(figsize=(10,10))
if not pfhr_flag:
    ax = sns.heatmap(TRX_PRR,cmap="RdYlGn",linewidths=0.6,vmin=0,vmax=1.0,square=True,cbar_kws={"shrink":0.83,"aspect":23,"pad":0.025},annot=True, annot_kws={"size": 8})
else:
    ax = sns.heatmap(TRX_PRR,cmap="RdYlGn",linewidths=0.6,vmin=0,vmax=1.0,square=True,cbar_kws={"shrink":0.498,"aspect":14,"pad":0.025},annot=True, annot_kws={"size": 8})
ax.collections[0].colorbar.set_label("UTX PRR",labelpad=8,fontsize=14,fontweight="regular")
ax.collections[0].colorbar.ax.set_frame_on(True)
ax.tick_params(axis='both',colors="black",width=1.6,length=4,tick1On=True)
ax.collections[0].colorbar.ax.tick_params(axis='both', colors="black", labelsize=11,width=1.6,length=4)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_visible(True)
    ax.spines[axis].set_color("black")
    ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_color("black")

y_labels = ['' for item in ax.get_yticklabels()]
ax.set_yticklabels(y_labels)
if not pfhr_flag:
    ax.set_ylabel("UTX PHY\n$O_{pp}$ [%]")
else:
    ax.set_ylabel("UTX PHY\n$E(O_{pfhr})$ [%]")
x_labels = ['' for item in ax.get_xticklabels()]
ax.set_xticklabels(x_labels)
ax.set_xlabel("$SIR$ [dB]\nInterferer PHY")

if not pfhr_flag:
    ax.hlines([5, 10, 15], *ax.get_xlim())
else:
    ax.hlines([3, 6, 9], *ax.get_xlim())
ax.vlines([5, 10, 15], *ax.get_ylim())


plt.gca().yaxis.get_label().set_fontsize(14)
plt.gca().yaxis.get_label().set_weight("regular")
plt.gca().yaxis.labelpad = 70
plt.gca().xaxis.get_label().set_fontsize(14)
plt.gca().xaxis.get_label().set_weight("regular")
plt.gca().xaxis.labelpad = 46

y_label_group(ax, TRX_PRR)
x_label_group(ax, TRX_PRR)
if not pfhr_flag:
    image_file = "PP_TRX_PRR.%s" % (extension)
else:
    image_file = "PFHR_TRX_PRR.%s" % (extension)
plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
plt.close()

fig = plt.figure(figsize=(10,10))
if not pfhr_flag:
    ax = sns.heatmap(IF_PRR,cmap="RdYlGn",linewidths=0.6,vmin=0,vmax=1.0,square=True,cbar_kws={"shrink":0.83,"aspect":23,"pad":0.025},annot=True, annot_kws={"size": 8})
else:
    ax = sns.heatmap(IF_PRR,cmap="RdYlGn",linewidths=0.6,vmin=0,vmax=1.0,square=True,cbar_kws={"shrink":0.498,"aspect":14,"pad":0.025},annot=True, annot_kws={"size": 8})
ax.collections[0].colorbar.set_label("Interferer PRR",labelpad=8,fontsize=14,fontweight="regular")
ax.collections[0].colorbar.ax.set_frame_on(True)
ax.tick_params(axis='both',colors="black",width=1.6,length=4,tick1On=True)
ax.collections[0].colorbar.ax.tick_params(axis='both', colors="black", labelsize=11,width=1.6,length=4)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_visible(True)
    ax.spines[axis].set_color("black")
    ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_color("black")

y_labels = ['' for item in ax.get_yticklabels()]
ax.set_yticklabels(y_labels)
if not pfhr_flag:
    ax.set_ylabel("UTX PHY\n$O_{pp}$ [%]")
else:
    ax.set_ylabel("UTX PHY\n$E(O_{pfhr})$ [%]")
x_labels = ['' for item in ax.get_xticklabels()]
ax.set_xticklabels(x_labels)
ax.set_xlabel("$SIR$ [dB]\nInterferer PHY")

if not pfhr_flag:
    ax.hlines([5, 10, 15], *ax.get_xlim())
else:
    ax.hlines([3, 6, 9], *ax.get_xlim())
ax.vlines([5, 10, 15], *ax.get_ylim())

plt.gca().yaxis.get_label().set_fontsize(14)
plt.gca().yaxis.get_label().set_weight("regular")
plt.gca().yaxis.labelpad = 70
plt.gca().xaxis.get_label().set_fontsize(14)
plt.gca().xaxis.get_label().set_weight("regular")
plt.gca().xaxis.labelpad = 46

y_label_group(ax, IF_PRR)
x_label_group(ax, IF_PRR)
if not pfhr_flag:
    image_file = "PP_IF_PRR.%s" % (extension)
else:
    image_file = "PFHR_IF_PRR.%s" % (extension)
plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
plt.close()
