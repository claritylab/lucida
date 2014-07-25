"""
Demo of a basic pie chart plus a few additional features.

In addition to the basic pie chart, this demo shows a few optional features:

    * slice labels
    * auto-labeling the percentage
    * offsetting a slice with "explode"
    * drop-shadow
    * custom start angle

Note about the custom start angle:

The default ``startangle`` is 0, which would start the "Frogs" slice on the
positive x-axis. This example sets ``startangle = 90`` such that everything is
rotated counter-clockwise by 90 degrees, and the frog slice starts on the
positive y-axis.
"""
import matplotlib.pyplot as plt

# The slices will be ordered and plotted counter-clockwise.
labels = ['GMM', 'HMM']
sizes = [85, 15]
colors = ['lightcoral', 'lightskyblue']
#explode = (0, 0, 0, 0) # only "explode" the 2nd slice (i.e. 'Hogs')

plt.pie(sizes, labels=labels, colors=colors, 
        autopct='%1.0f%%', shadow=True, startangle=45)
# Set aspect ratio to be equal so that pie is drawn as a circle.
plt.axis('equal')

plt.title("Speech Recognition (CMU Sphinx)\n")
plt.savefig("speech-breakdown.pdf")

plt.clf()

labels = ['Stemmer', 'Regex', 'CRF', 'Others']
sizes = [46, 22, 17, 15]
#colors = ['yellowgreen', 'gold', 'lightskyblue', 'lightcoral']
colors = ['lightcoral', 'gold', 'lightskyblue', 'yellowgreen']
explode = (0, 0, 0, 0.1) # only "explode" the 2nd slice (i.e. 'Hogs')

plt.pie(sizes, labels=labels, colors=colors, explode=explode,
        autopct='%1.0f%%', shadow=True, startangle=60)
# Set aspect ratio to be equal so that pie is drawn as a circle.
plt.axis('equal')

plt.title("Question-Answering (CMU OpenEphyra)\n")
plt.savefig("question-breakdown.pdf")

plt.clf()


labels = ['Feature Extraction', 'Feature Description', 'Others']
sizes = [42, 46, 12]
colors = ['gold', 'lightskyblue', 'yellowgreen']
explode = (0, 0, 0.1) # only "explode" the 2nd slice (i.e. 'Hogs')

plt.pie(sizes, labels=labels, colors=colors, explode=explode,
        autopct='%1.0f%%', shadow=True, startangle=60)
# Set aspect ratio to be equal so that pie is drawn as a circle.
plt.axis('equal')

plt.title("Image Processing (SURF)\n")
plt.savefig("image-breakdown.pdf")

plt.clf()


