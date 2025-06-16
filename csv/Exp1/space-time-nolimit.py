import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys



plt.rcParams.update({'font.size': 16})


bpt = [12.15, 7.30, 12.15, 7.30, 23.53, 14.61, 23.53, 14.61, 40.28, 35.42, 51.65, 42.74, 23.54, 15.85]
times = [44.2, 81.8, 42.9, 80.1, 46.3, 71.9, 43.1, 73.9, 44.5, 83.6, 46.3, 72.5, 22.5, 42.2]

labels = ['Ring-l', 'Ring-s', 'IRing-l', 'IRing-s', 'URing-l', 'URing-s', 'IURing-l', 'IURing-s', 'VRing-l', 
'VRing-s', 'VURing-l', 'VURing-s', 'RDFCSA-l', 'RDFCSA-s']

plt.figure(figsize=(8,6))
plt.scatter(bpt, times, c='black')
for i, txt in enumerate(labels):
	if txt in ['RDFCSA-s']:
		plt.annotate(txt, (bpt[i], times[i]-2.5), fontsize=14)
	elif txt in ['VURing-l']:
		plt.annotate(txt, (bpt[i]-6.4, times[i]), fontsize=14)
	elif txt in ['VRing-l']:
		plt.annotate(txt, (bpt[i]+0.5, times[i]-1.2), fontsize=14)
	elif txt in ['IRing-l']:
		plt.annotate(txt, (bpt[i]-5, times[i]-1.2), fontsize=14)
	elif txt in ['IRing-s', 'URing-s']:
		plt.annotate(txt, (bpt[i]+0.5, times[i]-1.2), fontsize=14)
	else:
		plt.annotate(txt, (bpt[i]+0.5, times[i]), fontsize=14)

plt.ylabel("Time (s)")
plt.xlabel("Space (bpt)")
plt.tight_layout()

plt.savefig('space-time-nolimit.pdf', bbox_inches ="tight")



