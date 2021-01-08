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

const int number_of_entries = 1068;

typedef struct{
    float call;
    float put;
} option;

void *thread_calculate_single_option(void *args){
    float *variables = (float*) args;
    float stock_price = variables[0], volatility = variables[1], risk_free_rate = variables[2], time = variables[3], strike_price = variables[4];
   
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

void calculate_options(float stock_price, float volatility, float time, float risk_free_rate){
    //Needed in the formula
    float divided_time = (time / (float)365);
    int lowest_strike_price, highest_strike_price;
    //Too high/low strike prices are not necessary
    lowest_strike_price = (int) floor(stock_price - (0.1 * stock_price));
    highest_strike_price = (int) floor(stock_price + (0.1 * stock_price));
    int strike_price;
    
    int number_of_options = highest_strike_price - lowest_strike_price;
    option option_prices[number_of_options];
    pthread_t threads[number_of_options];
    //For argument passing to thread function
    float variables[5];
    
    for (int i = 0; i < number_of_options; ++i)
    {
        strike_price = lowest_strike_price + i;
        variables[0] = stock_price;
        variables[1] = volatility;
        variables[2] = risk_free_rate;
        variables[3] = divided_time;
        variables[4] = strike_price;

        pthread_create(&threads[i], NULL, thread_calculate_single_option, (void *)variables); 
    }

    for (int i = 0; i < number_of_options; ++i)
    {
        //Get info from finished thread
        option *tmp;
        pthread_join(threads[i], (void**)&tmp);
        option_prices[i].call = tmp->call;
        option_prices[i].put = tmp->put;
        free(tmp);

        strike_price = lowest_strike_price + i;
        printf("Call\tStrike\tPut\n");
        printf("%.2f\t%d\t%.2f\n", option_prices[i].call, strike_price, option_prices[i].put);
        printf("----------------------\n");
    }
}

void run_python(char file_name[]){
    FILE* fp;

    fp = _Py_fopen(file_name, "r");
    PyRun_SimpleFile(fp, file_name);
}

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

    float current_price = 0, volatility, past_price;

    time_t raw_time;
    time(&raw_time);
    struct tm *time_info = localtime(&raw_time);

    float risk_free_rate, time;

    printf("Enter the current risk-free rate of interest:\n");
    scanf("%f", &risk_free_rate);

    printf("Duration of the option(in days):\n");
    scanf("%f", &time);

    do{
        run_python("get_current_stock_price.py");
        past_price = current_price;
        current_price = get_current_price();

        dequeue(&nodes.head, &nodes.tail);
        enqueue(&nodes.head, &nodes.tail, current_price);
        volatility = calculate_volatility(nodes.head);

        system("clear");

        calculate_options(current_price, volatility, time, risk_free_rate);
        
        printf("Price: %f\nVolatility: %f\n\n", current_price, volatility);

        //To limit Api requests and match data granularity
        sleep(5);

    //Working exchange market hours in local time
    } while( (time_info->tm_wday != 0 && time_info->tm_wday != 6) && 
        ( ((time_info->tm_hour >= 15 && time_info->tm_min >= 30) || time_info->tm_hour > 15) && time_info->tm_hour <= 22) );
    Py_Finalize();
}