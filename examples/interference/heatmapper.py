import pandas as pd
import numpy as np 
import seaborn as sns
import matplotlib.pyplot as plt

tx_raw = pd.read_csv("/home/relsas/RIOT-benpicco/examples/interference/TX_120B_OF_-1440US_SIR_0DB.csv",header=None)
tx_raw.columns = ["TRX PHY","IF PHY","PRR"]
tx_raw.replace({"SUN-OFDM 863-870MHz ":""},regex=True,inplace=True)

tx_matrix = tx_raw.pivot("TRX PHY","IF PHY","PRR")

plt.figure(figsize=(6,6))
ax = sns.heatmap(tx_matrix,cmap="Blues",linewidths=1.2,linecolor="lightgrey",vmin=0,vmax=1.0,annot=True,square=True,cbar_kws={"shrink":0.85,"label":"PRR"})
plt.title("Heatmap",fontsize=17)
plt.gca().yaxis.get_label().set_fontsize(14)
plt.gca().xaxis.get_label().set_fontsize(14)
plt.yticks(rotation=45,va="center")
plt.xticks(rotation=45,ha="center") 
plt.savefig("test.png",bbox_inches='tight',dpi=330)
plt.close()