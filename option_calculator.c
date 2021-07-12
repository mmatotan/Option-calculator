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
int number_of_tickers, number_of_dates, number_of_threads, number_of_active_threads = 0;
char **tickers, *active_threads;
float *days;

//Structure for end values of an option
typedef struct{
    float call;
    float put;
} option;

pthread_mutex_t thread_mutex;
pthread_cond_t thread_cond;

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

    pthread_mutex_lock(&thread_mutex);
    number_of_active_threads--;
    pthread_cond_signal(&thread_cond);
    pthread_mutex_unlock(&thread_mutex);

    pthread_exit(option_price);
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

int main(int argc, char *argv[]){

    //Arguments setup for number of threads
    if(argc == 2){
        number_of_threads = atoi(argv[1]);
        printf("Running the program on %d threads.\n", number_of_threads);
    } else {
        number_of_threads = 1;
        printf("Running the program on %d thread.\n", number_of_threads);
    }

    //Setup array that keeps track of running threads
    active_threads = (char *) malloc(number_of_threads * sizeof(char));
    for(int i = 0; i < number_of_threads; i++) active_threads[i] = 0;

    Py_Initialize();

    get_tickers();
    get_dates();

    time_t raw_time;
    time(&raw_time);
    struct tm *time_info;
    
    float risk_free_rate, divided_time, current_price, volatility;
    float *variables;
    option *option_prices[number_of_tickers][number_of_dates];
    int number_of_options[number_of_tickers], lowest_strikes[number_of_tickers];
    FILE *fp;
    
    pthread_mutex_init(&thread_mutex, NULL);
    pthread_cond_init(&thread_cond, NULL);

    do{
        run_python("IV_and_stock_pc.py");
        time_info = localtime(&raw_time);
        
        fp = fopen("variables.csv", "r");

        fscanf(fp, "%f\n", &risk_free_rate);

        clock_t begin = clock();

        for(int i = 0; i < number_of_tickers; i++){
            //Get data
            fscanf(fp, "%f,%f\n", &current_price, &volatility);

            int lowest_strike_price, highest_strike_price;
            //Prepare the needed strike prices which the stock may be able to reach based on it's volatility
            lowest_strike_price = (int) floor(current_price - ((volatility / 100) * current_price));
            if(lowest_strike_price < 1) lowest_strike_price = 1; //Stocks can't hit negative value
            highest_strike_price = (int) floor(current_price + ((volatility / 100) * current_price));

            number_of_options[i] = highest_strike_price - lowest_strike_price;
            lowest_strikes[i] = lowest_strike_price;

            for(int j = 0; j < number_of_dates; j++){
                
                pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * number_of_options[i]);

                option_prices[i][j] = (option *) malloc(sizeof(option) * number_of_options[i]);

                for(int k = lowest_strike_price; k < highest_strike_price; k++){

                    variables = (float *) malloc(5 * sizeof(float));

                    variables[0] = current_price;
                    variables[1] = volatility / 100; //From % to decimal
                    variables[2] = risk_free_rate;
                    variables[3] = k;
                    variables[4] = days[j] / 365; //To a fraction of a year

                    pthread_mutex_lock(&thread_mutex);

                    if(number_of_active_threads == number_of_threads){
                        pthread_cond_wait(&thread_cond, &thread_mutex);
                    }
                    pthread_create(&threads[k - lowest_strike_price], NULL, thread_calculate_single_option, (void *)variables);
                    number_of_active_threads++;

                    pthread_mutex_unlock(&thread_mutex);
                }

                for(int k = 0; k < number_of_options[i]; k++){
                    option *tmp;
                    pthread_join(threads[k], (void**)&tmp);
                    option_prices[i][j][k].call = tmp->call;
                    option_prices[i][j][k].put = tmp->put;
                    free(tmp);
                }

                free(threads);
            }
        }

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("\nTimed: %f seconds\n", time_spent);

        fclose(fp);

        for(int i = 0; i < number_of_tickers; i++){
            char *ticker = malloc(strlen(tickers[i]) * sizeof(char));
            strcpy(ticker, tickers[i]);
            strcat(ticker, ".csv");
            FILE *fp = fopen(ticker, "a");
            free(ticker);

            for(int j = 0; j < number_of_dates; j++){
                fprintf(fp, "\n%d,%d/%d/%d,%d:%d:%d\n", (int)days[j], time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900, time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
                for(int k = 0; k < number_of_options[i]; k++){
                    fprintf(fp, "%.2f,%d,%.2f\n", option_prices[i][j][k].call, lowest_strikes[i] + k, option_prices[i][j][k].put);
                }
            }
            fclose(fp);
        }

        break;

    //Working exchange market hours in local time
    } while( (time_info->tm_wday != 0 && time_info->tm_wday != 6) && 
        ( ((time_info->tm_hour >= 15 && time_info->tm_min >= 30) || time_info->tm_hour > 15) && time_info->tm_hour <= 22) );

    Py_Finalize();
}