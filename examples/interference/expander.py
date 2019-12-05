import glob
import pandas as pd
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--sources", nargs="*", help="The csv sources to combine into the destination csv file")
parser.add_argument("-d", "--destination", help="The destination csv file in which to combine all sources")
args = parser.parse_args()
print(args)

files = args.sources
df = pd.concat([pd.read_csv(f) for f in files]).drop_duplicates()
df.to_csv("merged.csv", index=False)