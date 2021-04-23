#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <python3.8/Python.h>
#include <unistd.h>
#include <pthread.h>
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

pthread_mutex_t options_mutex;

void *thread_calculate_single_option(void *args){
    //Fetch needed variables for option calculation and free args
    float *variables = (float*) args;

    float stock_price = variables[0], volatility = variables[1], risk_free_rate = variables[2], strike_price = variables[3], time = variables[4];
    
    free(args);

    //The rest follows the formula for calculating options, all functions are located in calculations.h
    float d1 = calculate_d1(stock_price, strike_price, risk_free_rate, volatility, time);
    float d2 = calculate_d2(d1, volatility, time);

    float Nd1 = normalCDF(d1);
    float Nd2 = normalCDF(d2);

    option *option_price = malloc(sizeof(option));

    option_price->call = calculate_call_option(stock_price, risk_free_rate, time, Nd1, Nd2, strike_price);
    //To simulate real world, options won't ever be free for purchase, they may expire worthless though
    if(option_price->call < 0.01) option_price->call = 0.01;

    option_price->put = calculate_put_option(option_price->call, strike_price, risk_free_rate, time, stock_price);
    //Same for put
    if(option_price->put < 0.01) option_price->put = 0.01;

    pthread_exit(option_price);
}

void *thread_calculate_options(void *args){
    //Fetch needed variables and free args
    float *variables = (float*) args;
    
    float stock_price = variables[0], volatility = variables[1], risk_free_rate = variables[2], strike_price, dte = variables[4];
    float *data_to_send;

    int id_of_ticker = (int) variables[5];
    time_t raw_time;
    time(&raw_time);
    struct tm *time_info;

    free(args);
    
    int lowest_strike_price, highest_strike_price;
    //Prepare the needed strike prices which the stock may be able to reach based on it's volatility
    lowest_strike_price = (int) floor(stock_price - (volatility * stock_price));
    if(lowest_strike_price < 1) lowest_strike_price = 1; //Stocks can't hit negative value
    highest_strike_price = (int) floor(stock_price + (volatility * stock_price));
    
    int number_of_options = highest_strike_price - lowest_strike_price;
    option option_prices[number_of_options];
    //Each option calculation gets its own thread
    pthread_t threads[number_of_options];

    for (int i = 0; i < number_of_options; i++)
    {
        //Allocate data to send for further processing, it will be freed when the thread is done fetching variables
        data_to_send = (float *)malloc(5 * sizeof(float));

        data_to_send[0] = stock_price;
        data_to_send[1] = volatility;
        data_to_send[2] = risk_free_rate;
        strike_price = (float)lowest_strike_price + i;
        data_to_send[3] = strike_price;
        data_to_send[4] = dte;

        pthread_create(&threads[i], NULL, thread_calculate_single_option, (void *)data_to_send); 
    }
    
    //Join threads and get results
    for(int i = 0; i < number_of_options; i++){
        option *tmp;
        pthread_join(threads[i], (void**)&tmp);
        option_prices[i].call = tmp->call;
        option_prices[i].put = tmp->put;
        free(tmp);
    }

    time_info = localtime(&raw_time);

    //Lock mutex for output so results don't mix and store the results in seperate .csv files
    pthread_mutex_lock(&options_mutex);

    char *ticker = malloc(strlen(tickers[id_of_ticker]) * sizeof(char));
    strcpy(ticker, tickers[id_of_ticker]);
    strcat(ticker, ".csv");
    FILE *fp = fopen(ticker, "a");
    free(ticker);
    fprintf(fp, "\n%d,%d/%d/%d,%d:%d:%d\n", (int)(dte * 365), time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900, time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    for(int i = 0; i < number_of_options; i++){
        fprintf(fp, "%.2f,%d,%.2f\n", option_prices[i].call, lowest_strike_price + i, option_prices[i].put);
    }
    fclose(fp);

    pthread_mutex_unlock(&options_mutex);

    pthread_exit(0);
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
        printf("Enter the ticker:\n");
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
    printf("For how many dates would you like to calculate options? ");
    scanf("%d", &number_of_dates);
    days = malloc(number_of_dates * sizeof(float));

    for(int i = 0; i < number_of_dates; i++){
        printf("Enter the number of days:\n");
        scanf("%f", &days[i]);
    }

    return;
}

int main(){
    Py_Initialize();

    get_tickers();
    get_dates();

    time_t raw_time;
    time(&raw_time);
    struct tm *time_info;
    
    float risk_free_rate, divided_time, current_price, volatility;
    float *variables;
    
    pthread_mutex_init(&options_mutex, NULL);

    do{
        run_python("IV_and_stock_pc.py");
        time_info = localtime(&raw_time);
        
        pthread_t threads[number_of_tickers][number_of_dates];
        clock_t begin = clock();
        
        FILE *fp = fopen("variables.csv", "r");

        fscanf(fp, "%f\n", &risk_free_rate);

        for(int i = 0; i < number_of_tickers; i++){
            //Get data
            fscanf(fp, "%f,%f\n", &current_price, &volatility);

            for(int j = 0; j < number_of_dates; j++){
                variables = (float *) malloc(6 * sizeof(float));

                variables[0] = current_price;
                variables[1] = volatility / 100; //From % to decimal
                variables[2] = risk_free_rate;
                variables[3] = 0; //Strike price not yet determined
                variables[4] = days[j] / 365;
                variables[5] = i;

                pthread_create(&threads[i][j], NULL, thread_calculate_options, (void *)variables);
            }
        }

        fclose(fp);

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("\nTimed: %f seconds\n", time_spent);

        break;

    //Working exchange market hours in local time
    } while( (time_info->tm_wday != 0 && time_info->tm_wday != 6) && 
        ( ((time_info->tm_hour >= 15 && time_info->tm_min >= 30) || time_info->tm_hour > 15) && time_info->tm_hour <= 22) );

    Py_Finalize();
}