import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt

extension = "pdf"

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_-1440US_SIR_0DB.csv",header=None)
tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_matrix = tx_raw.pivot("TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR")

plt.figure(figsize=(6,6))

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="lightgrey",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.8,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.tick_params(axis='both', colors='lightgrey', labelsize=11,width=1.8,length=7)

plt.title("Packet Reception Ratio under 21B\nACK interference @ -1.44 ms offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
plt.gca().xaxis.labelpad = 10
plt.yticks(rotation=45,va="center")
plt.xticks(rotation=45,ha="center") 

my_colors = ['g', 'g', 'r', 'r']

for ticklabel, tickcolor in zip(plt.gca().get_xticklabels(), my_colors):
    ticklabel.set_color(tickcolor)
for ticklabel, tickcolor in zip(plt.gca().get_yticklabels(), my_colors):
    ticklabel.set_color(tickcolor)

plt.savefig("begin.%s" % extension,bbox_inches='tight',dpi=330)
plt.close()

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_3120US_SIR_0DB.csv",header=None)
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_3960US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_7920US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_8760US_SIR_0DB.csv",header=None)])
tx_raw.reset_index(drop=True,inplace=True)

tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_matrix = tx_raw.pivot("TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR")

plt.figure(figsize=(6,6))

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="lightgrey",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.8,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.tick_params(axis='both', colors='lightgrey', labelsize=11,width=1.8,length=7)

plt.title("Packet Reception Ratio under 21B\nACK interference @ middle offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
plt.gca().xaxis.labelpad = 10
plt.yticks(rotation=45,va="center")
plt.xticks(rotation=45,ha="center") 

my_colors = ['g', 'g', 'r', 'r']

for ticklabel, tickcolor in zip(plt.gca().get_xticklabels(), my_colors):
    ticklabel.set_color(tickcolor)
for ticklabel, tickcolor in zip(plt.gca().get_yticklabels(), my_colors):
    ticklabel.set_color(tickcolor)

plt.savefig("middle.%s" % extension,bbox_inches='tight',dpi=330)
plt.close()

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_6720US_SIR_0DB.csv",header=None)
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_8400US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_16320US_SIR_0DB.csv",header=None)])
tx_raw = pd.concat([tx_raw,pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_18000US_SIR_0DB.csv",header=None)])
tx_raw.reset_index(drop=True,inplace=True)

tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_matrix = tx_raw.pivot("TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR")

plt.figure(figsize=(6,6))

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="lightgrey",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.8,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.tick_params(axis='both', colors='lightgrey', labelsize=11,width=1.8,length=7)

plt.title("Packet Reception Ratio under 21B\nACK interference @ end offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
plt.gca().xaxis.labelpad = 10
plt.yticks(rotation=45,va="center")
plt.xticks(rotation=45,ha="center") 

my_colors = ['g', 'g', 'r', 'r']

for ticklabel, tickcolor in zip(plt.gca().get_xticklabels(), my_colors):
    ticklabel.set_color(tickcolor)
for ticklabel, tickcolor in zip(plt.gca().get_yticklabels(), my_colors):
    ticklabel.set_color(tickcolor)

plt.savefig("end.%s" % extension,bbox_inches='tight',dpi=330)
plt.close()
