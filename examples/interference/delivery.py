import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
from scipy import interpolate
from numpy import nan as NaN

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

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_22B_TX_120B_OF_3720US_SIR_0DB.csv",header=None)
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_22B_TX_120B_OF_4620US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_22B_TX_120B_OF_8520US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/IF_22B_TX_120B_OF_9420US_SIR_0DB.csv",header=None)])
tx_raw.reset_index(drop=True,inplace=True)

tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.insert(0,"TX / RX payload",120)
tx_raw.insert(0,"Interferer payload",22)
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_complete = pd.concat([tx_complete,tx_raw])
tx_complete.reset_index(drop=True,inplace=True)

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

tx_complete = tx_complete[["Interferer payload","TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]]

trx_phy_list = sorted(tx_complete["TX / RX PHY\nconfiguration"].unique().tolist())

fig, axes = plt.subplots(nrows=2, ncols=2)
coord = {0:(0,0),1:(0,1),2:(1,0),3:(1,1)}
cmap = cm.get_cmap('Spectral')

for index, trx_phy in enumerate(trx_phy_list):
    dfp = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Interferer payload", columns="Interferer PHY\nconfiguration", values="PRR")
    dfp_i = tx_complete.loc[tx_complete["TX / RX PHY\nconfiguration"] == trx_phy].pivot(index="Interferer payload", columns="Interferer PHY\nconfiguration", values="PRR")
    for i in np.arange(21.0,23.0,0.05):
        if not i in dfp_i.index:
            dfp_i.loc[i] = [NaN,NaN,NaN,NaN]
    dfp_i = dfp_i.sort_index().interpolate(method="krogh")

    dfp.plot(ax=axes[coord[index][0],coord[index][1]],marker='x',markeredgewidth=1.8,markersize=8,linestyle="None",legend=False,colormap=cmap)
    ax = dfp_i.plot(ax=axes[coord[index][0],coord[index][1]],linewidth=1.5,legend=False,colormap=cmap)
    axes[coord[index][0],coord[index][1]].set_xlim(20.95,23.05)
    axes[coord[index][0],coord[index][1]].set_xticks([21,22,23])
    axes[coord[index][0],coord[index][1]].set_ylim(-0.05,1.05)
    axes[coord[index][0],coord[index][1]].set_ylabel("PRR")

handles, labels = ax.get_legend_handles_labels()
print(handles[4:8])
fig.legend(handles[4:8], labels[4:8], loc="lower center",ncol=4)

plt.show()