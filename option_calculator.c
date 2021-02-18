#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <python3.8/Python.h>
#include <unistd.h>
#include <pthread.h>
#include "calculations.h"
#include "nodes.h"

//Number of stock prices tracked, has to match get_intraday_data.py
const int number_of_entries = 120;

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

    //Rest follows the formula for calculating options, all functions are in calculations.h
    float d1 = calculate_d1(stock_price, strike_price, risk_free_rate, volatility, time);
    float d2 = calculate_d2(d1, volatility, time);

    float Nd1 = normalCDF(d1);
    float Nd2 = normalCDF(d2);

    option *option_price = malloc(sizeof(option));

    option_price->call = calculate_call_option(stock_price, risk_free_rate, time, Nd1, Nd2, strike_price);
    //To simulate real world, options won't ever be free
    if(option_price->call < 0.03) option_price->call = 0.03;

    option_price->put = calculate_put_option(option_price->call, strike_price, risk_free_rate, time, stock_price);
    //Same for put
    if(option_price->put < 0.03) option_price->put = 0.03;

    pthread_exit(option_price);
}

void *thread_calculate_options(void *args){
    //Fetch needed variables and free args
    float *variables = (float*) args;
    
    float stock_price = variables[0], volatility = variables[1], risk_free_rate = variables[2], strike_price, time = variables[4];
    float *data_to_send;

    free(args);
    
    int lowest_strike_price, highest_strike_price;
    //Too high/low strike prices are not necessary
    lowest_strike_price = (int) floor(stock_price - (0.1 * stock_price));
    highest_strike_price = (int) floor(stock_price + (0.1 * stock_price));
    
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
        data_to_send[4] = time;

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

    //Lock mutex for output so results don't mix and output the results
    pthread_mutex_lock(&options_mutex);
    printf("\nFOR DURATION OF %d DAYS\n", (int)(time * 365));
    for(int i = 0; i < number_of_options; i++){
        
        printf("Call\tStrike\tPut\n");
        printf("%.2f\t%d\t%.2f\n", option_prices[i].call, lowest_strike_price + i, option_prices[i].put);
        printf("----------------------\n");
    }
    pthread_mutex_unlock(&options_mutex);

    pthread_exit(0);
}

//Embeds provided python file and runs it, needed for APIs used
void run_python(char file_name[]){
    FILE* fp;

    fp = _Py_fopen(file_name, "r");
    PyRun_SimpleFile(fp, file_name);
}

//Gets intraday data and aranges it in a queue
Queue init(){
    run_python("get_intraday_data.py");
    Node *head = NULL, *tail = NULL;
    FILE *fp = fopen("intraday_data.txt", "r");
    float number;
    int i = 0;

    while(fscanf(fp, "%f", &number) > 0 && i < number_of_entries)
    {
        enqueue(&head, &tail, number);
        i++;
    }

    Queue nodes;
    nodes.head = head;
    nodes.tail = tail;
    return nodes;
}

void get_ticker(){
    char ticker[10];
    printf("Enter the ticker:\n");
    scanf("%s", ticker);
    FILE *fp = fopen("ticker.txt", "w");
    fprintf(fp, "%s", ticker);
    fclose(fp);
    printf("Loading data for %s\n", ticker);
}

float get_current_price(){
    FILE *fp = fopen("current_stock_price.txt", "r");
    float price;
    fscanf(fp, "%f", &price);
    return price;
}

float calculate_volatility(Node *head){
    float mean = 0, sum_of_squared_deviations = 0;
    Node *t;

    if(!head) return -1;

    for(t = head; t != NULL; t = t -> next){
        mean += (t -> val);
    }

    mean /= (float) number_of_entries;
    
    for(t = head; t != NULL; t = t -> next){
        sum_of_squared_deviations += pow(((t -> val) - mean), 2);
    }

    float variance = sum_of_squared_deviations / number_of_entries;
    float standardDeviation = sqrt(variance);

    return standardDeviation;
}

int main(){
    Py_Initialize();
    get_ticker();

    //Loads historical data from api
    Queue nodes = init();

    time_t raw_time;
    time(&raw_time);
    struct tm *time_info = localtime(&raw_time);

    //Most often used option durations
    float days[3] = {90, 180, 270};
    float risk_free_rate, divided_time, current_price = 0, volatility;
    float *variables;

    printf("Enter the current risk-free rate of interest:\n");
    scanf("%f", &risk_free_rate);
    
    pthread_mutex_init(&options_mutex, NULL);

    do{
        //CLR
        system("clear");
        pthread_t threads[3];

        run_python("get_current_stock_price.py");
        current_price = get_current_price();

        //FIFO - Removes the last stock price entry and places a fresh one on top
        dequeue(&nodes.head, &nodes.tail);
        enqueue(&nodes.head, &nodes.tail, current_price);
        volatility = calculate_volatility(nodes.head);

        clock_t begin = clock();
        for (int i = 0; i < 3; i++)
        {
            //Allocate data to send for further processing, it will be freed when the thread is done fetching variables
            variables = (float *) malloc(5 * sizeof(float));

            variables[0] = current_price;
            variables[1] = volatility;
            variables[2] = risk_free_rate;
            variables[3] = 0; //Strike price not yet determined
            divided_time = (days[i] / 365);
            variables[4] = divided_time;

            pthread_create(&threads[i], NULL, thread_calculate_options, (void *)variables);
        }
        clock_t end = clock();

        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

        //To limit Api requests and match data granularity the program sleeps
        sleep(1);
        printf("\nTimed: %f seconds", time_spent);
        printf("\nCurrent Stock Price: %f\nVolatility: %f\n", current_price, volatility);
        sleep(4);

    //Working exchange market hours in local time
    } while( (time_info->tm_wday != 0 && time_info->tm_wday != 6) && 
        ( ((time_info->tm_hour >= 15 && time_info->tm_min >= 30) || time_info->tm_hour > 15) && time_info->tm_hour <= 22) );

    Py_Finalize();
}