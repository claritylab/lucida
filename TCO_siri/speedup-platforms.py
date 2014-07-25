
import matplotlib.pyplot as plt; plt.rcdefaults()
import numpy as np
import matplotlib.pyplot as plt

platform = ('Multi-core CPU', 'GPU', '8-core TAU', '16-core TAU', 'Intel Phi')
y_pos = np.arange(len(platform))
speech_perf = [3.2, 18.98, 0.53, 0.83, 1.09]

plt.bar(y_pos, speech_perf, align='center', alpha=0.6)
plt.xticks(y_pos, platform)
#plt.yticks(1)
plt.ylabel('Speedup')
plt.title('Speech recognition speedup on each platform')

plt.savefig("speech-speedup.pdf")

plt.clf()

platform = ('Multi-core CPU', 'GPU', '8-core TAU', '16-core TAU', 'Intel Phi')
y_pos = np.arange(len(platform))
question_perf = [2.72, 3.66, 1.84, 2.31, 3.46]

plt.bar(y_pos, question_perf, align='center', alpha=0.6)
plt.xticks(y_pos, platform)
#plt.yticks(1)
plt.ylabel('Speedup')
plt.title('Question-answering speedup on each platform')

plt.savefig("question-speedup.pdf")

plt.clf()

platform = ('Multi-core CPU', 'GPU', '8-core TAU', '16-core TAU', 'Intel Phi')
y_pos = np.arange(len(platform))
image_perf = [3.58, 6.03, 0.79, 0.80, 3.08]

plt.bar(y_pos, image_perf, align='center', alpha=0.6)
plt.xticks(y_pos, platform)
#plt.yticks(1)
plt.ylabel('Speedup')
plt.title('Image processing speedup on each platform')

plt.savefig("image-speedup.pdf")

plt.clf()

cpu_servers = 33300.00
cpu_tco = 218537331.84	

gpu_servers = 24600.00
gpu_tco = 219588381.44	

phi_servers = 44400.00
phi_tco = 423279747.32	

cost_plat = [250, 259, 40, 80, 2437]
power_plat = [80, 170, 4.8, 9.6, 225]

#for plat in ('GPU', 'Intel Phi'):
for s in ('speech', 'question', 'image'):

   plt.title('Performance / TCO compared to Multicore CPU server')
   
   

   plt.savefig("perf-tco-"+s+".pdf")

   plt.clf()
   
for plat in ('8-core TAU', '16-core TAU', 'ASIC'):

   plt.title('Performance / Watt compared to Multicore CPU server')

   plt.savefig("perf-watt-"+plat+".pdf")

   plt.clf()   
   


