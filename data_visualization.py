import numpy as np
from scipy.stats import iqr
import matplotlib.pyplot as plt
import csv

raw_x = []
raw_y = []
sorted_y = []

x = []
y = []

x_minimum = 0
y_minimum = 10

x_maximum = 0
y_maximum = 0

with open('results.csv', 'r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        raw_x.append(int(row[0]))
        raw_y.append(float(row[1]))
        sorted_y.append(float(row[1]))

    sorted_y.sort()
    Q1 = np.quantile(raw_y, 0.25)
    Q3 = np.quantile(raw_y, 0.75)
    IQR = Q3 - Q1

    lower = Q1 - 1.25*IQR
    upper = Q3 + 1.25*IQR

    #mean, dev = np.mean(raw_y), np.std(raw_y)
    #cut_off = dev * 3
    #lower, upper = mean - cut_off, mean + cut_off
    
    for i in raw_x:
        if(i != 1 and (raw_y[i - 1] < lower or raw_y[i - 1] > upper)):
            continue

        x.append(i)
        y.append(raw_y[i - 1])

        if(raw_y[i - 1] < y_minimum):
            y_minimum = float(raw_y[i - 1])
            x_minimum = i
        if(float(raw_y[i - 1]) > y_maximum):
            y_maximum = float(raw_y[i - 1])
            x_maximum = i

print(f"Minimum na {x_minimum} dretvi/dretve")
print(f"Maximum: {x_maximum} dretvi/dretve")

plt.plot(x, y, 'kx', label="Zasebno vrijeme izračuna")
plt.plot(x_minimum, y_minimum, 'ro', label="Najbrže vrijeme")
plt.plot(x_maximum, y_maximum, 'go', label="Najduže vrijeme")

stupanj = 2
reg = np.polyfit(x, y, stupanj)
f = np.poly1d(reg)
plt.plot(x, f(x), lw=3, label=f"Regresijska krivulja {stupanj}. stupnja", c='r')

plt.xlabel("Broj dretvi")
plt.ylabel("Vremena izračuna")
plt.legend()
plt.show()