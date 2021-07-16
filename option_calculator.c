#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <python3.8/Python.h>
#include <unistd.h>
#include <omp.h>
#include "calculations.h"

//Global variables which are only changed in functions get_tickers() and get_dates()
//They are used in multiple other functions and threads
int number_of_tickers, number_of_dates;
char **tickers;
float *days;

//Structure for end values of an option
typedef struct{
    float call;
    float put;
} option;

option calculate_single_option(float stock_price, float volatility, float risk_free_rate, float strike_price, float time){

    //The rest follows the formula for calculating options, all functions are located in calculations.h
    float d1 = calculate_d1(stock_price, strike_price, risk_free_rate, volatility, time);
    float d2 = calculate_d2(d1, volatility, time);

    float Nd1 = normalCDF(d1);
    float Nd2 = normalCDF(d2);

    option option_price;

    option_price.call = calculate_call_option(stock_price, risk_free_rate, time, Nd1, Nd2, strike_price);
    //To simulate real world, options won't ever be free for purchase, they may expire worthless though
    if(option_price.call < 0.01) option_price.call = 0.01;

    option_price.put = calculate_put_option(option_price.call, strike_price, risk_free_rate, time, stock_price);
    //Same for put
    if(option_price.put < 0.01) option_price.put = 0.01;

    return option_price;
}

//Embeds provided python file and runs it, needed for APIs used
void run_python(char file_name[]){

    FILE* fp = _Py_fopen(file_name, "r");
    PyRun_SimpleFile(fp, file_name);
    fclose(fp);

    return;
}

void get_tickers(){
    //Ticker selection and preparation to send to the Python script
    printf("How many tickers would you like to follow? ");
    scanf("%d", &number_of_tickers);
    char ticker[8];
    tickers = malloc(number_of_tickers * sizeof(char *));
    FILE *fp = fopen("tickers.csv", "w");
    
    for(int i = 0; i < number_of_tickers; i++){
        printf("\nEnter the %d. ticker: ", i + 1);
        scanf("%s", ticker);
        fprintf(fp, "%s", ticker);
        if(i != number_of_tickers - 1) fprintf(fp, "\n");

        //Store tickers
        tickers[i] = malloc(strlen(ticker) * sizeof(char));
        strcpy(tickers[i], ticker);

        strcat(ticker, ".csv");
        FILE *csv = fopen(ticker, "w");
        fclose(csv);
    }
    
    fclose(fp);

    return;
}

void get_dates(){
    //Options usually exipre on fridays but it is left to the user's will to put in their own DTE(days till expiry)
    printf("\nFor how many dates would you like to calculate options? ");
    scanf("%d", &number_of_dates);
    days = malloc(number_of_dates * sizeof(float));

    for(int i = 0; i < number_of_dates; i++){
        printf("\nEnter the %d. DTE: ", i + 1);
        scanf("%f", &days[i]);
    }

    return;
}

int main(int argc, char *argv[]){

    int max_threads;

    //Arguments setup for number of threads
    if(argc == 2){
        //If running on >1 threads
        if(atoi(argv[1]) != 1){
            max_threads = atoi(argv[1]);
            omp_set_nested(1);        
        } else {
            max_threads = 1;
            omp_set_nested(0);    
        }
    } else {
        max_threads = 1;
        omp_set_nested(0);
    }
    
    //Python embed init, input for tickers and dates
    Py_Initialize();
    get_tickers();
    get_dates();

    //Time to check if NYSE is working currently
    time_t raw_time;
    time(&raw_time);
    struct tm *time_info;
    
    float risk_free_rate, current_price, volatility, current_prices[number_of_tickers], volatilities[number_of_tickers];
    int number_of_strikes[number_of_tickers], lowest_strikes[number_of_tickers], highest_strikes[number_of_tickers], cache_skip;
    unsigned long long int number_of_calculations = 0;
    option *option_prices[number_of_tickers][number_of_dates];
    double dtime;

    do{
        cache_skip = 5;

        //run_python("IV_and_stock_pc.py");
        time_info = localtime(&raw_time);
        
        FILE *fp = fopen("variables.csv", "r");

        fscanf(fp, "%f\n", &risk_free_rate);

        //Loop for loading and preparing values
        for (int i = 0; i < number_of_tickers; i++)
        {
            fscanf(fp, "%f,%f\n", &current_prices[i], &volatilities[i]);
            
            lowest_strikes[i] = (int) floor(current_prices[i] - ((volatilities[i] / 100) * current_prices[i]));
            if(lowest_strikes[i] < 1) lowest_strikes[i] = 1;
            highest_strikes[i] = (int) floor(current_prices[i] + ((volatilities[i] / 100) * current_prices[i]));
            number_of_strikes[i] = highest_strikes[i] - lowest_strikes[i];
            
            number_of_calculations += (number_of_dates * number_of_strikes[i]);
        }

        fclose(fp);

        fp = fopen("results.csv", "w");

        for (int threads = 1; threads <= max_threads; threads++)
        {
            omp_set_num_threads(threads);
        
            dtime = omp_get_wtime();

            int i, j, k;
            //Parralelized loop for calculating and saving options
            #pragma omp parallel for private(j, k) schedule(nonmonotonic:dynamic) collapse(2)
            for (i = 0; i < number_of_tickers; i++)
            {
                for (j = 0; j < number_of_dates; j++)
                {
                    option *chain = malloc(number_of_strikes[i] * sizeof(option));
                    for (k = 0; k < number_of_strikes[i]; k++)
                    {
                        chain[k] = calculate_single_option(current_prices[i], volatilities[i] / 100, risk_free_rate, k + lowest_strikes[i], days[j] / 365);
                    }
                    option_prices[i][j] = chain;
                }   
            }

            dtime = omp_get_wtime() - dtime;

            //First calculation will always be the slowest since the CPU uses cached values for all next calculations
            //To keep integrity of data the first calculation is processed but not recorded
            if(cache_skip > 0 && threads == 1){
                cache_skip = 0;
                threads--;
                continue;
            }

            fprintf(fp, "%d,%f,%lld\n", threads, dtime, number_of_calculations);

            printf("\nTimed: %f seconds for %lld calculations on %d thread(s)\n", dtime, number_of_calculations, threads);
        }

        fclose(fp);
        
        //Loops for saving prices in .csv files
        for(int i = 0; i < number_of_tickers; i++){

            char *ticker = malloc(strlen(tickers[i]) * sizeof(char));
            strcpy(ticker, tickers[i]);
            strcat(ticker, ".csv");
            FILE *csv = fopen(ticker, "a");
            free(ticker);

            for(int j = 0; j < number_of_dates; j++){

                fprintf(csv, "\n%d,%d/%d/%d,%d:%d:%d\n", (int)days[j], time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900, time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

                for(int k = 0; k < number_of_strikes[i]; k++){
                    fprintf(csv, "%.2f,%d,%.2f\n", option_prices[i][j][k].call, k + lowest_strikes[i], option_prices[i][j][k].put);
                }

                free(option_prices[i][j]);
            }

            fclose(csv);
        }

        //For constant updating delete this break
        break;

    //NYSE working hours in local time
    } while( (time_info->tm_wday != 0 && time_info->tm_wday != 6) && ( ((time_info->tm_hour >= 15 && time_info->tm_min >= 30) || time_info->tm_hour > 15) && time_info->tm_hour <= 22) );

    run_python("data_visualization.py");

    Py_Finalize();

    return 0;
}