"""
Bar chart demo with pairs of bars grouped for easy comparison.
"""
import numpy as np
import matplotlib.pyplot as plt


n_groups = 6

speech = (20, 35, 30, 35, 27)

question = (25, 32, 34, 20, 25)

image 

fig, ax = plt.subplots()

index = np.arange(n_groups)
bar_width = 0.35

opacity = 0.4
error_config = {'ecolor': '0.3'}

rects1 = plt.bar(index, speech, bar_width,
                 alpha=opacity,
                 color='b',
                 label='Speech Recognition')

rects2 = plt.bar(index + bar_width, question, bar_width,
                 alpha=opacity,
                 color='r',
                 label='Question Anwering')

rects2 = plt.bar(index + bar_width, image, bar_width,
                 alpha=opacity,
                 color='y',
                 label='Image Matching')
                 
plt.xlabel('Acceleration Platforms')
plt.ylabel('Performance / TCO improvement')
#plt.title('Performance / TCO across acceleration platforms')
plt.xticks(index + bar_width, ('Multicore CPU', 'GPU', '8-core TAU', '16-core TAU', 'Intel Phi'))
plt.legend()

plt.tight_layout()

plt.savefig("perf-tco-platforms.pdf")


