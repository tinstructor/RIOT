import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
from scipy import interpolate
from numpy import nan as NaN
import math
import random

def get_payload_overlap(phy_tuple,payload_tuple):
    # NOTE phy_tuple(if_idx,trx_idx)
    # NOTE payload_tuple(if_payload_size,trx_payload_size)
    udbps = {2:6,3:12,4:6,5:12}
    tail_bits = 6
    pfhr_sym = 12
    if_sym = (math.ceil(((payload_tuple[0] * 8) + tail_bits) / udbps[phy_tuple[0]]) + pfhr_sym)
    trx_sym = (math.ceil(((payload_tuple[1] * 8) + tail_bits) / udbps[phy_tuple[1]]) + pfhr_sym)
    return (-1 if trx_sym - pfhr_sym < if_sym else if_sym / (trx_sym - pfhr_sym))

extension = "png"
transparent_flag = False

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_21B_TX_120B_OF_3840US_SIR_0DB.csv",header=None)
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_21B_TX_120B_OF_4680US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_21B_TX_120B_OF_8640US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_21B_TX_120B_OF_9480US_SIR_0DB.csv",header=None)])
tx_raw.reset_index(drop=True,inplace=True)

tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.insert(0,"TX / RX payload",120)
tx_raw.insert(0,"Interferer payload",21)
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_complete = tx_raw

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_23B_TX_120B_OF_3660US_SIR_0DB.csv",header=None)
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_23B_TX_120B_OF_4620US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_23B_TX_120B_OF_8460US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_23B_TX_120B_OF_9420US_SIR_0DB.csv",header=None)])
tx_raw.reset_index(drop=True,inplace=True)

tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.insert(0,"TX / RX payload",120)
tx_raw.insert(0,"Interferer payload",23)
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)

# NOTE the following data is fake and serves merely to develop
tx_raw["Interferer payload"] = 25
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 27
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 29
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 31
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 33
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 35
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 36
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 38
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 40
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)
tx_raw["Interferer payload"] = 42
tx_raw["PRR"] = tx_complete.apply(lambda row: random.random(),axis=1)
tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)

phy_names = {"O4 MCS2":2,"O4 MCS3":3,"O3 MCS1":4,"O3 MCS2":5}
tx_complete["Payload overlap"] = tx_complete.apply(lambda row: get_payload_overlap((phy_names[row["Interferer PHY\nconfiguration"]],phy_names[row["TX / RX PHY\nconfiguration"]]),(row["Interferer payload"],row["TX / RX payload"])),axis=1)
tx_complete["Weighted PRR"] = tx_complete["PRR"] * tx_complete["Payload overlap"]

# print(tx_complete)

trx_phy_list = sorted(tx_complete["TX / RX PHY\nconfiguration"].unique().tolist())

fig, axes = plt.subplots(nrows=2, ncols=2)
coord = {0:(0,0),1:(0,1),2:(1,0),3:(1,1)}
cmap = cm.get_cmap('Spectral')

for index, trx_phy in enumerate(trx_phy_list):
    # print(tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy])
    dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Payload overlap", columns="Interferer PHY\nconfiguration", values="PRR")
    # print(dfp)
    dfp_i = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Payload overlap", columns="Interferer PHY\nconfiguration", values="PRR")
    for i in np.arange(dfp_i.index.min(),dfp_i.index.max(),0.0025):
        if not i in dfp_i.index:
            dfp_i.loc[i] = [NaN,NaN,NaN,NaN]
    dfp_i = dfp_i.sort_index().interpolate(method="quadratic",limit_area="inside")

    dfp.plot(ax=axes[coord[index][0],coord[index][1]],marker='x',markeredgewidth=1.8,markersize=8,linestyle="None",legend=False,colormap=cmap)
    ax = dfp_i.plot(ax=axes[coord[index][0],coord[index][1]],linewidth=1.5,legend=False,colormap=cmap)
    axes[coord[index][0],coord[index][1]].set_xticks(np.arange(0.0,1.0,0.02 if coord[index][1] == 0 else 0.04).tolist())
    axes[coord[index][0],coord[index][1]].set_xlim(dfp_i.index.min()-0.01,dfp_i.index.max()+0.02)
    axes[coord[index][0],coord[index][1]].set_ylim(-0.05,1.05)
    axes[coord[index][0],coord[index][1]].set_ylabel("PRR")
    axes[coord[index][0],coord[index][1]].title.set_text("TX / RX PHY: " + trx_phy)

handles, labels = ax.get_legend_handles_labels()
fig.legend(handles[4:8], labels[4:8], loc="lower center",ncol=4)

plt.show()