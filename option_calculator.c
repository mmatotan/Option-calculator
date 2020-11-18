#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <python3.8/Python.h>

const float e = 2.71828;

typedef struct{
    float call;
    float put;
} option;

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

option calculate_single_option(float stock_price, float volatility, float risk_free_rate, float time, float strike_price, option option_price){
    float d1 = calculate_d1(stock_price, strike_price, risk_free_rate, volatility, time);
    float d2 = calculate_d2(d1, volatility, time);

    float Nd1 = normalCDF(d1);
    float Nd2 = normalCDF(d2);

    option_price.call = calculate_call_option(stock_price, risk_free_rate, time, Nd1, Nd2, strike_price);
    option_price.put = calculate_put_option(option_price.call, strike_price, risk_free_rate, time, stock_price);

    return option_price;
}

void run_python_script(char data_retrieve[], FILE* fp){
    Py_Initialize();

    fp = _Py_fopen(data_retrieve, "r");
    PyRun_SimpleFile(fp, data_retrieve);

    Py_Finalize();
}

void calculate_options(/*Insert data from python script*/){
    float stock_price, volatility, risk_free_rate, time;

    // Should be replaced with automatic values gathered from an API, lower values also
    printf("Enter the current stock price:\n");
    scanf("%f", &stock_price);

    // Should be replaced with a calculated volatility
    printf("Enter the current vloatility:\n");
    scanf("%f", &volatility);

    // Calculated by subtracting the current inflation rate from the yield of the Treasury bond
    printf("Enter the current risk-free rate of interest:\n");
    scanf("%f", &risk_free_rate);

    printf("Duration of the option(in days):\n");
    scanf("%f", &time);
    time = (time / (float)365);

    int lowest_strike_price, highest_strike_price;
    lowest_strike_price = (int) floor(stock_price - (0.25 * stock_price));
    highest_strike_price = (int) floor(stock_price + (0.25 * stock_price));
    int strike_price;
    option option_prices[highest_strike_price - lowest_strike_price];
    
    for (int i = 0; i < highest_strike_price - lowest_strike_price; ++i)
    {
        strike_price = lowest_strike_price + i;
        option_prices[i] = calculate_single_option(stock_price, volatility, risk_free_rate, time, strike_price, option_prices[i]);
        printf("Strike price: %d\n", strike_price);
        printf("Call option price: %f\n", option_prices[i].call);
        printf("Put option price: %f\n", option_prices[i].put);
        printf("-----------------------------------\n");
    }
}

int main(){
    int exit_check;
    char data_retrieve[] = "get_info.py";
    FILE* fp;

    do{
        //run_python_script(data_retrieve, fp);

        //get data from txt

        calculate_options(/*Insert data from python script*/);

        printf("Continue?\n0 - exit\n1 - continue");
        scanf("%d", &exit_check);
    } while(exit_check == 1);

}