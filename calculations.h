#ifndef CALCULATIONS_H_
# define CALCULATIONS_H_

#include <math.h>
#define e 2.71828

float calculate_call_option(float stock_price, float risk_free_rate, float time, float Nd1, float Nd2, float strike_price){
    float Vc;
    Vc = stock_price * Nd1 - ((strike_price / pow(e, risk_free_rate * time)) * Nd2);
    return Vc;
}

float calculate_put_option(float Vc, float strike_price, float risk_free_rate, float time, float stock_price){
    float Vp;
    Vp = Vc + (strike_price / pow(e, risk_free_rate * time)) - stock_price;
    return Vp;
}

float calculate_d1(float stock_price, float strike_price, float risk_free_rate, float volatility, float time){
    float d1;
    d1 = (log(stock_price / strike_price) + (risk_free_rate + (0.5 * pow(volatility, 2))) * time) / (volatility * sqrt(time));
    return d1;
}

float calculate_d2(float d1, float volatility, float time){
    float d2;
    d2 = d1 - (volatility * sqrt(time));
    return d2;
}

// Calculates the standard normal value, eliminates the use of hard-coded values from the standard normal value table
double normalCDF(double d)
{
   return 0.5 * erfc(-d * M_SQRT1_2);
}

#endif