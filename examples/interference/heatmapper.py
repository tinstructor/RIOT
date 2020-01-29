import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt

extension = "png"

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_-1440US_SIR_0DB.csv",header=None)
tx_raw.columns = ["TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR"]
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_matrix = tx_raw.pivot("TX / RX PHY\nconfiguration","Interferer PHY\nconfiguration","PRR")

plt.figure(figsize=(6,6))

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="#a9a9a9",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.804,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.collections[0].colorbar.ax.set_frame_on(True)
ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)
ax.collections[0].colorbar.ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_visible(True)
    ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
    ax.spines[axis].set_color('#a9a9a9')
    ax.collections[0].colorbar.ax.spines[axis].set_color('#a9a9a9')

plt.title("Packet Reception Ratio under 21B\nACK interference @ -1.44 ms offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
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

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="#a9a9a9",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.804,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.collections[0].colorbar.ax.set_frame_on(True)
ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)
ax.collections[0].colorbar.ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_visible(True)
    ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
    ax.spines[axis].set_color('#a9a9a9')
    ax.collections[0].colorbar.ax.spines[axis].set_color('#a9a9a9')

plt.title("Packet Reception Ratio under 21B\nACK interference @ middle offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
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

ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.4,linecolor="#a9a9a9",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.804,"aspect":14})
ax.collections[0].colorbar.set_label("PRR",labelpad=8,fontsize=11)
ax.collections[0].colorbar.ax.set_frame_on(True)
ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)
ax.collections[0].colorbar.ax.tick_params(axis='both', colors="#a9a9a9", labelsize=11,width=1.6,length=7)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_visible(True)
    ax.spines[axis].set_linewidth(1.6)
    ax.collections[0].colorbar.ax.spines[axis].set_linewidth(1.6)
    ax.spines[axis].set_color('#a9a9a9')
    ax.collections[0].colorbar.ax.spines[axis].set_color('#a9a9a9')

plt.title("Packet Reception Ratio under 21B\nACK interference @ end offset",fontsize=15,pad=16)
plt.gca().yaxis.get_label().set_fontsize(11)
plt.gca().yaxis.labelpad = 10
plt.gca().xaxis.get_label().set_fontsize(11)
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

plt.savefig("end.%s" % extension,bbox_inches='tight',dpi=330)
plt.close()
