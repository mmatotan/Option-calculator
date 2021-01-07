from alpha_vantage.timeseries import TimeSeries
import numpy as np
# Aplha Vantaga key: OW2YG7NAMNK8U8CZ

size = 90 #Equal to minutes
granularity = 12

ts = TimeSeries(key='OW2YG7NAMNK8U8CZ', output_format='pandas')

t = open('ticker.txt', 'r')
ticker = t.read()
t.close()

data = ts.get_intraday(symbol=ticker, outputsize='compact', interval='1min')
close_price = data[0]['4. close']

f = open('intraday_data.txt', 'w')

for i in range(size - 1):
    minute = np.linspace(close_price[-(size - i)], close_price[-(size - i) + 1], granularity)
    for j in range(granularity):
        f.write(str(round(minute[j], 2)))
        f.write("\n")
    
f.close()