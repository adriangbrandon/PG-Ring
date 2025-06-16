import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



plt.rcParams.update({'font.size': 16})


bpt = [12.15, 7.30, 12.15, 7.30, 23.53, 14.61, 23.53, 14.61, 40.28, 35.42, 51.65, 42.74, 23.54, 15.85]
times = [414, 1173, 176, 696, 373, 872, 161, 353, 1265, 3198, 1059, 1727, 390, 1248]

labels = ['Ring-l', 'Ring-s', 'IRing-l', 'IRing-s', 'URing-l', 'URing-s', 'IURing-l', 'IURing-s', 'VRing-l', 
'VRing-s', 'VURing-l', 'VURing-s', 'RDFCSA-l', 'RDFCSA-s']

plt.figure(figsize=(8,6))
plt.scatter(bpt, times, c='black')
for i, txt in enumerate(labels):
	if txt in ['IURing-s', 'URing-l', 'IRing-l', 'IURing-l']:
		plt.annotate(txt, (bpt[i]+0.5, times[i]-100), fontsize=14)
	elif txt in ['Ring-l', 'RDFCSA-l']:
		plt.annotate(txt, (bpt[i]+0.5, times[i]+50), fontsize=14)
	elif txt in ['VURing-l']:
		plt.annotate(txt, (bpt[i]-6.5, times[i]), fontsize=14)
	else:
		plt.annotate(txt, (bpt[i]+0.5, times[i]), fontsize=14)

plt.ylabel("Time (ms)")
plt.xlabel("Space (bpt)")
plt.tight_layout()

plt.savefig('space-time.pdf', bbox_inches ="tight")



