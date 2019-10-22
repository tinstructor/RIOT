import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import re
import glob
import os
import sys

file_list = []
script_dir = os.path.dirname(__file__)
for file in glob.glob(os.path.join(script_dir,"*.csv")):
    file_list.append(file)

REGEXP = re.compile(r"^.*_(?P<bytes>\d+)B.*_(?P<time>\d+)(?P<prefix>[UM]*)S\.csv$")

df = pd.read_csv(file_list[0], header=None)
df.columns = ["TRX PHY","IF PHY","PRR"]
if_phys = df["IF PHY"].unique()

graph_info = {}

for csv_file in file_list:
    match = re.match(REGEXP, csv_file)

    payload_size = "B"
    offset = "S"

    if (match):
        payload_size = match.group("bytes") + payload_size
        offset = match.group("time") + match.group("prefix") + offset

        df = pd.read_csv(csv_file, header=None)
        df.columns = ["TRX PHY","IF PHY","PRR"]
        df["PRR"] *= 100

        for if_phy in if_phys:
            df_if_phy = df.loc[df["IF PHY"] == if_phy]
            
            if (if_phy, payload_size not in graph_info):
                graph_info.setdefault((if_phy, payload_size), {})
            
            if (offset not in graph_info[if_phy, payload_size]):
                graph_info[if_phy, payload_size].setdefault(offset, {})

            graph_info[if_phy, payload_size][offset].update(df_if_phy[["TRX PHY","PRR"]].set_index("TRX PHY")["PRR"].to_dict())

    else:
        raise ValueError("CSV filename %s incorrectly formatted" % csv_file)

for if_phy, payload_size in graph_info:
    title = "Interference: " + if_phy + ", Payload: " + payload_size
    graph_info[if_phy, payload_size] = dict(sorted(graph_info[if_phy, payload_size].items(), key=lambda x: x[0].lower()))
    ax = pd.DataFrame(graph_info[if_phy, payload_size]).T.plot(kind='bar', figsize=(10,7))
    ax.set_xlabel('Offset between TX and IF')
    ax.set_ylabel('Packet Reception Rate [%]')
    ax.set_ylim([0, 120])
    ax.set_yticks([0,20,40,60,80,100])
    for i in ax.patches:
        ax.text(i.get_x() + i.get_width() / 2, i.get_height()+1.2, str(round(i.get_height(),2)), fontsize=8, color='dimgrey', rotation=90, ha="center", va="bottom")
    plt.title(title)
    plt.savefig(if_phy + "_" + payload_size + ".png")
