from yahoo_fin import stock_info as si

t = open('ticker.txt', 'r')
ticker = t.read()
t.close()

f = open('current_stock_price.txt', 'w')
f.write(str(si.get_live_price(ticker)))
f.close()