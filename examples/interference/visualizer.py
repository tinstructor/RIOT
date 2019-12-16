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

REGEXP = re.compile(r"^.*_(?P<bytes>\d+)B.*_(?P<time>\d+)(?P<prefix>[UM]*)S.*_(?P<sir>-?\d{1,2})DB\.csv$")

frame_list = []

# NOTE make a frame_list from all csv files with correctly formatted name
for csv_file in file_list:
    match = re.match(REGEXP, csv_file)
    if (match):
        foo = pd.read_csv(csv_file, header=None)
        foo.columns = ["TRX PHY","IF PHY","PRR"]
        frame_list.append(foo)

# NOTE concatenate all frames from frame_list into single frame before selecting
# the IF PHY column and only keeping unique values in if_phys
if_phys = pd.concat(frame_list, axis=0, ignore_index=True)["IF PHY"].unique()

bar_info = {}
line_info = {}

for csv_file in file_list:
    match = re.match(REGEXP, csv_file)

    payload_size = "B"
    offset = "S"
    sir = "DB"

    if (match):
        payload_size = match.group("bytes") + payload_size
        offset = match.group("time") + match.group("prefix") + offset
        sir = match.group("sir") + sir

        df = pd.read_csv(csv_file, header=None)
        df.columns = ["TRX PHY","IF PHY","PRR"]
        df["PRR"] *= 100

        for if_phy in if_phys:
            # NOTE selects all rows whose column value equals if_phy
            df_if_phy = df.loc[df["IF PHY"] == if_phy]
            
            if (if_phy, payload_size, sir not in bar_info):
                bar_info.setdefault((if_phy, payload_size, sir), {})
            
            if (offset not in bar_info[if_phy, payload_size, sir]):
                bar_info[if_phy, payload_size, sir].setdefault(offset, {})

            # NOTE selects TRX PHY and PRR columns from df_if_phy and sets TRX PHY as index before converting the frame
            # to a dict with key = TRX PHY and value = PRR and finally appending to bar_info[if_phy, payload_size, sir][offset]
            bar_info[if_phy, payload_size, sir][offset].update(df_if_phy[["TRX PHY","PRR"]].set_index("TRX PHY")["PRR"].to_dict())

            if (if_phy, payload_size, offset not in line_info):
                line_info.setdefault((if_phy, payload_size, offset), {})

            if (sir not in line_info[if_phy, payload_size, offset]):
                line_info[if_phy, payload_size, offset].setdefault(sir, {})

            # NOTE selects TRX PHY and PRR columns from df_if_phy and sets TRX PHY as index before converting the frame
            # to a dict with key = TRX PHY and value = PRR and finally appending to line_info[if_phy, payload_size, offset][sir]
            line_info[if_phy, payload_size, offset][sir].update(df_if_phy[["TRX PHY","PRR"]].set_index("TRX PHY")["PRR"].to_dict())
   
    # REVIEW removing else clause allows other csv files in directory
    else:
        raise ValueError("CSV filename %s incorrectly formatted" % csv_file)

# NOTE loops over each (if_phy, payload_size, sir) tuple in bar_info
for if_phy, payload_size, sir in bar_info:
    title = "Interference: " + if_phy + ", Payload: " + payload_size + ", SIR: " + sir
    # NOTE sort (alfabetically) by offset
    bar_info[if_phy, payload_size, sir] = dict(sorted(bar_info[if_phy, payload_size, sir].items(), key=lambda x: x[0].lower()))
    # NOTE first transpose the dataframe (i.e., reflect the dataframe over its main 
    # diagonal by writing rows as columns and vice-versa) and plot it as a bar plot
    ax = pd.DataFrame(bar_info[if_phy, payload_size, sir]).T.plot(kind='bar', figsize=(10,7))
    ax.set_xlabel('Offset between TX and IF')
    ax.set_ylabel('Packet Reception Rate [%]')
    ax.set_ylim([0, 120])
    ax.set_yticks([0,20,40,60,80,100])
    # NOTE for each bar: print the height of each bar 1.2 above the top of the bar
    # tags are centered and rotated 90 degrees counter-clockwise
    for i in ax.patches:
        ax.text(i.get_x() + i.get_width() / 2, i.get_height()+1.2, str(round(i.get_height(),2)), fontsize=8, color='dimgrey', rotation=90, ha="center", va="bottom")
    plt.title(title)
    plt.savefig(if_phy + "_" + payload_size + "_" + sir + ".png")
    plt.close()

styles = ['s-', 'o-', '^-', 'x-', 'd-', 'h-']
# NOTE loops over each (if_phy, payload_size, offset) tuple in line_info
for if_phy, payload_size, offset in line_info:
    title = "Interference: " + if_phy + ", Payload: " + payload_size + ", Offset: " + offset
    # NOTE sort (alfabetically) by sir
    line_info[if_phy, payload_size, offset] = dict(sorted(line_info[if_phy, payload_size, offset].items(), key=lambda x: x[0].lower()))
    # NOTE first transpose the dataframe (i.e., reflect the dataframe over its main 
    # diagonal by writing rows as columns and vice-versa) and plot it as a line plot
    df = pd.DataFrame(line_info[if_phy, payload_size, offset]).T
    ax = df.plot(kind='line', figsize=(10,7), style=styles, xticks=range(len(list(df.index.values))))
    ax.legend(loc='lower right')
    ax.set_xlabel('SIR between TX and IF')
    ax.set_ylabel('Packet Reception Rate [%]')
    ax.set_ylim([0, 120])
    ax.set_yticks([0,20,40,60,80,100])
    ax.set_xlim([-0.05,len(list(df.index.values)) - 0.95])
    plt.title(title)
    plt.savefig(if_phy + "_" + payload_size + "_" + offset + ".png")
    plt.close()