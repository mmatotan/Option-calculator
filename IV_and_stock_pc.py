from yahoo_fin import options
from yahoo_fin import stock_info as si
import numpy as np
import math as m
from scipy.stats import norm as nd
import datetime
import pandas
import csv

option_selector = 0

#Fetch 10y bond rate
r = si.get_live_price("^TNX")
if m.isnan(r):
    r = 0.01356
else:
    r = np.round(r / 100, 4)

tickers = []
csvFile = open("tickers.csv", "r")
csvReader = csv.reader(csvFile)
for row in csvReader:
    tickers.append(row)
csvFile.close()

class OptionPrice(object):
    def __init__(self, stock_price, strike_price, time_to_exp_days, risk_free_rate_pc):

        self.S = stock_price
        self.K = strike_price
        self.t = time_to_exp_days
        self.r = risk_free_rate_pc

    def calc_d1(self):
        return (m.log(self.S / self.K) + (self.r + self.v**2 / 2) * self.t) / (self.v * m.sqrt(self.t))

    def calc_d2(self):
        d1 = self.calc_d1()
        return d1 - self.v * m.sqrt(self.t)
 
    def call_price(self, annual_vol_pc):
        self.v = annual_vol_pc
        d1 = self.calc_d1()
        d2 = self.calc_d2()
        callprice = self.S * nd.cdf(d1) - self.K * m.exp(-self.r * self.t) * nd.cdf(d2)
        return np.round(callprice, 2)

csvFile = open("variables.csv", "w")
csvWriter = csv.writer(csvFile)
csvWriter.writerow([r])

for ticker in tickers:

    option = options.get_calls(ticker[0]).sort_values("Last Trade Date", ascending=False)

    dateString = option.iat[option_selector, 0]
    dateString = dateString.split(ticker[0])[1][:6]
    date = datetime.datetime(2000 + int(dateString[:2]), int(dateString[2:4]), int(dateString[4:6]), 22, 00, 00)
    #DTE - Days Till Expiry
    DTE = np.round(((date - datetime.datetime.now()).days + 1) / 365, 4)

    stockPrice = np.round(si.get_live_price(ticker[0]), 2)

    strikePrice = option.iat[option_selector, 2]

    price = OptionPrice(stockPrice, strikePrice, DTE, r)
    last_price = option.iat[option_selector, 3]

    #Metoda bisekcije
    low_IV = 0
    high_IV = 2000
    IV = (low_IV + high_IV) / 2

    guess_price = price.call_price(IV / 100)
    while guess_price != last_price:
        if guess_price < last_price:
            low_IV = IV
        else:
            high_IV = IV
        IV = (low_IV + high_IV) / 2
        IV = np.round(IV, 2)
        guess_price = price.call_price(IV / 100)

    csvWriter.writerow([stockPrice] + [IV])

csvFile.close()