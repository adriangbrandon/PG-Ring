import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes, mark_inset
import numpy as np


def create_exponential_list_with_max(base, start, max_value):
    result = []
    exponent = start
    while True:
        value = base ** exponent
        if value > max_value:
            break
        result.append(value)
        exponent += 1
    return result


# Parameters
bucket_length = 100
max_limit = 1000000
min_cnt = 1
log_base = 2
file_path = 'data.csv'  # Replace with your file path


# Step 1: Read the file
data_adaptive = pd.read_csv('adaptive/type3.cltj.csv', sep=';', header=None, names=['id', 'res', 'time'])
data_fixed = pd.read_csv('fixed/type3.cltj.csv', sep=';', header=None, names=['id', 'res', 'time'])

data_adaptive['time']= data_adaptive['time']/1000000000.0
data_fixed['time'] = data_fixed['time']/1000000000.0

#data_adaptive = data_adaptive[data_adaptive['res'] <= max_limit]
#data_fixed = data_fixed[data_fixed['res'] <= max_limit]
bucket_lengths=create_exponential_list_with_max(log_base, 2, 100000000000)


bucket_boundaries = [2] + np.cumsum(bucket_lengths).tolist()  # Compute boundaries
print(bucket_boundaries)

# Step 2: Assign buckets
#data_adaptive['bucket'] = data_adaptive['res'] // bucket_length
#data_fixed['bucket'] = data_fixed['res'] // bucket_length

data_adaptive['bucket'] = pd.cut(data_adaptive['res'], bins=bucket_boundaries, labels=range(len(bucket_lengths)), right=False)
data_fixed['bucket'] = pd.cut(data_fixed['res'], bins=bucket_boundaries, labels=range(len(bucket_lengths)), right=False)

# Step 3: Compute bucket averages
bucket_stats_ad = data_adaptive.groupby('bucket').agg(
    avg=('time', 'mean'),
    count=('time', 'size')
).reset_index()
#bucket_averages_ad['bucket_mid'] = bucket_averages_ad['bucket'] * bucket_length + bucket_length / 2
bucket_stats_ad['bucket_mid'] = [
    bucket_boundaries[i] for i in range(len(bucket_lengths))
]
bucket_stats_ad = bucket_stats_ad[bucket_stats_ad['count'] >= min_cnt]

bucket_stats_fix = data_fixed.groupby('bucket').agg(
    avg=('time', 'mean'),
    count=('time', 'size')
).reset_index()
#bucket_averages_fix['bucket_mid'] = bucket_averages_fix['bucket'] * bucket_length + bucket_length / 2
#bucket_averages_fix = bucket_averages_fix[bucket_averages_fix['count'] >= min_cnt]
bucket_stats_fix['bucket_mid'] = [
    bucket_boundaries[i] for i in range(len(bucket_lengths))
]
bucket_stats_fix = bucket_stats_fix[bucket_stats_fix['count'] >= min_cnt]


fig, ax1 = plt.subplots(figsize=(10, 6))

# Step 4: Plot the results
#ax1.plot(bucket_averages_ad['bucket_mid'], bucket_averages_ad['avg'], linestyle='-', color='blue')
#ax1.plot(bucket_averages_fix['bucket_mid'], bucket_averages_fix['avg'], linestyle='--', color='red')
ax1.plot(bucket_stats_ad['bucket_mid'], bucket_stats_ad['avg'], marker='o', linestyle='-', color='blue')
ax1.plot(bucket_stats_fix['bucket_mid'], bucket_stats_fix['avg'], marker='o', linestyle='--',color='red')
#plt.bar(bucket_averages_ad['bucket_mid'], bucket_averages_ad['avg'], width=bucket_length * 0.9, color='blue', alpha=0.4)
#plt.bar(bucket_averages_fix['bucket_mid'], bucket_averages_fix['avg'], width=bucket_length * 0.9, color='red', alpha=0.4)
#for idx, row in bucket_averages_ad.iterrows():
#    plt.text(row['bucket_mid'], row['avg'], row['count'], ha='center', va='top', fontsize=9)

# Step 6: Add inset axes
# Define the inset axes
ax_inset = inset_axes(ax1, width="39%", height="40%", loc="upper left", borderpad=2.5)
#ax_inset = inset_axes(ax1, width="39%", height="40%", loc="upper left", borderpad=2.5)

# Plot the zoomed-in region on the inset
zoom_start, zoom_end = 1, 10000  # Define the zoom range for x-axis
zoom_data_ad = bucket_stats_ad[
    (bucket_stats_ad['bucket_mid'] >= zoom_start) & (bucket_stats_ad['bucket_mid'] <= zoom_end)
]
zoom_data_fix = bucket_stats_fix[
    (bucket_stats_fix['bucket_mid'] >= zoom_start) & (bucket_stats_fix['bucket_mid'] <= zoom_end)
]

# Add the inset line and bar plots
ax_inset.plot(zoom_data_ad['bucket_mid'], zoom_data_ad['avg'], marker='o', linestyle='-', color='blue')
ax_inset.plot(zoom_data_fix['bucket_mid'], zoom_data_fix['avg'], marker='o', linestyle='--', color='red')

#print(bucket_stats_ad)
#print(bucket_stats_fix)


# Set the limits for the inset
ax_inset.set_xlim(zoom_start, zoom_end)
#ax_inset.set_ylim(-1, 90)
#ax_inset.set_ylim(-1, 25)
ax_inset.set_ylim(-2, 30)
ax_inset.set_xscale('log', base=log_base)
ax1.set_xscale('log', base=log_base)

xticks = [2**i for i in range(0, 33, 2)] 
xtick_labels = [f"$2^{{{i}}}$" for i in range(0, 33, 2)]  # Label only whole powers of 2
ax1.set_xticks(xticks, xtick_labels)
#mark_inset(ax1, ax_inset, loc1=2, loc2=4, fc="none", ec="grey", lw=1)

ax1.set_xlabel('Results')
ax1.set_ylabel('Averaged time (sec.)')
plt.show()
