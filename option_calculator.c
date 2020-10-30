#include <stdio.h>
#include <math.h>
#include <stdlib.h>

const float e = 2.71828;

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

float calculate_d1(float stock_price, float strike_price, float risk_free_rate, float sigma, float time){
    float d1;
    d1 = (log(stock_price / strike_price) + (risk_free_rate + (0.5 * pow(sigma, 2))) * time) / (sigma * sqrt(time));
    return d1;
}

float calculate_d2(float d1, float sigma, float time){
    float d2;
    d2 = d1 - (sigma * sqrt(time));
    return d2;
}

double normalCDF(double d)
{
   return 0.5 * erfc(-d * M_SQRT1_2);
}

int main(){
    float stock_price, volatility, risk_free_rate, strike_price, time;
    printf("Enter the current stock price:\n");
    scanf("%f", &stock_price);
    printf("Enter the current implied vloatility:\n");
    scanf("%f", &volatility);
    printf("Enter the current risk-free rate of interest:\n");
    scanf("%f", &risk_free_rate);
    printf("Enter the strike price you wish to calculate:\n");
    scanf("%f", &strike_price);
    printf("Duration of the option(in days):\n");
    scanf("%f", &time);
    time = time / 365;
    float d1 = calculate_d1(stock_price, strike_price, risk_free_rate, volatility, time);
    float d2 = calculate_d2(d1, volatility, time);
    float Nd1 = normalCDF(d1);
    float Nd2 = normalCDF(d2);
    float call_price = calculate_call_option(stock_price, risk_free_rate, time, Nd1, Nd2, strike_price);
    float put_price = calculate_put_option(call_price, strike_price, risk_free_rate, time, stock_price);
    printf("Call option price: %f\n", call_price);
    printf("Put option price: %f\n", put_price);
}