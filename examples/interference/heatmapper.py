import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt
import math
import os.path

sns.set(rc={"font.family":"sans-serif","font.weight":"regular","font.sans-serif":["Fira Sans"]})

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

extension = "png"
legacy_flag = False
transparent_flag = False
trx_payload_size = 120 # in bytes
if_payload_sizes = [20,25,30,35,40] # in bytes

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
    
    tx_raw.reset_index(drop=True,inplace=True)
    if legacy_flag:
        tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
    else:
        tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR","foo"]
    tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

    tx_matrix = tx_raw.pivot("TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR")

    plt.figure(figsize=(6,6))

    ax = sns.heatmap(tx_matrix,cmap="Reds",linewidths=1.4,linecolor="#a9a9a9",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.804,"aspect":14})
    ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11,fontweight="regular")
    ax.collections[0].colorbar.ax.set_frame_on(True)
    ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7,tick1On=True)
    ax.collections[0].colorbar.ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)

    for axis in ['top','bottom','left','right']:
        ax.spines[axis].set_visible(True)
        ax.spines[axis].set_linewidth(1.6)
        ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
        ax.spines[axis].set_color('#a9a9a9')
        ax.collections[0].colorbar.ax.spines[axis].set_color('#a9a9a9')

    plt.title("Packet Reception Ratio\nunder %dB ACK interference" % if_payload_size,fontsize=15,pad=16,fontweight="regular")
    plt.gca().yaxis.get_label().set_fontsize(11)
    plt.gca().yaxis.get_label().set_weight("regular")
    plt.gca().yaxis.labelpad = 10
    plt.gca().xaxis.get_label().set_fontsize(11)
    plt.gca().xaxis.get_label().set_weight("regular")
    plt.gca().xaxis.labelpad = 10
    plt.yticks(rotation=45,va="center")
    plt.xticks(rotation=45,ha="center") 

    my_colors = ['#707070', '#707070', '#707070', '#707070']

    for ticklabel, tickcolor in zip(plt.gca().get_xticklabels(), my_colors):
        ticklabel.set_color(tickcolor)
    for ticklabel, tickcolor in zip(plt.gca().get_yticklabels(), my_colors):
        ticklabel.set_color(tickcolor)

    cbar_tick_color = "#707070"
    for ticklabel in ax.collections[0].colorbar.ax.get_yticklabels():
        ticklabel.set_color(cbar_tick_color)

    image_file = "IF_%dB_TX_%dB.%s" % (if_payload_size,trx_payload_size,extension)
    plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
    plt.close()
