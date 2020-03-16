import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt
import math
import os.path

sns.set(rc={"font.family":"sans-serif","font.weight":"regular","font.sans-serif":["Fira Sans"]})

extension = "png"
legacy_flag = False
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

if not pfhr_flag:
    for if_payload_size in if_payload_sizes:
        a = [2,3,4,5]
        b = [2,3,4,5]
        index_tuples = [(x,y) for x in a for y in b]
        offset_list = [get_offset_list((if_idx,trx_idx),(if_payload_size,trx_payload_size))[0] for if_idx,trx_idx in index_tuples]
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
elif not legacy_flag:
    tx_complete = pd.DataFrame()
    for if_payload_size in if_payload_sizes:
        a = [2,3,4,5]
        b = [2,3,4,5]
        index_tuples = [(x,y) for x in a for y in b]
        temp_list = [get_offset_list((if_idx,trx_idx),(if_payload_size,trx_payload_size)) for if_idx,trx_idx in index_tuples]
        offset_list = sum(temp_list,[])
        offset_list = sorted(list(set(offset_list)))
        
        tx_raw = pd.DataFrame()
        for offset in offset_list:
            filename = "/home/relsas/RIOT-benpicco/examples/interference/PFHR_IF_%dB_TX_%dB_OF_%dUS_SIR_0DB.csv"%(if_payload_size,trx_payload_size,offset)
            if os.path.isfile(filename):
                if tx_raw.empty:
                    tx_raw = pd.read_csv(filename,header=None)
                    tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR","foo"]
                    tx_raw.insert(0,"Offset",offset)
                else:
                    temp_df = pd.read_csv(filename,header=None)
                    temp_df.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR","foo"]
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
    overlap_list = sorted(tx_complete["Payload overlap"].unique().tolist())

    for index, overlap in enumerate(overlap_list):
        temp_dfp = tx_complete.loc[tx_complete["Payload overlap"] == overlap].copy()
        print(temp_dfp)
        tx_matrix = temp_dfp.pivot_table(index=["TX / RX PHY\nconfiguration"],columns=["Interferer PHY\nconfiguration"],values=["PRR"])
        print(tx_matrix)
        plt.figure(figsize=(6,6))

        ax = sns.heatmap(tx_matrix["PRR"],cmap="Reds",linewidths=1.4,linecolor="#a9a9a9",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.804,"aspect":14})
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

        image_file = "PFHR_IF_%dB_TX_%dB_OL_%.3f.%s" % (if_payload_size,trx_payload_size,overlap,extension)
        plt.savefig(image_file,bbox_inches='tight',dpi=330,transparent=transparent_flag)
        plt.close()
