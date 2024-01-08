#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>
#include <math.h>
#include "weather_data.h"

#define API_ENDPOINT "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s"
#define Error_Memory_Allocation -2
#define Error_JSON_Parsing -3
#define Error_File_Opening -4

#define temperature_high 60.0
#define temperature_low 0.0
#define humidity_high 80
#define wind_speed 25
void append_data(struct WeatherInfo *data, double temperature, double humidity, double pressure); //function declarartion

static const char* runtime_file= "run_times.txt";

static int run_time = 0;


CURL *f_handler;
CURLcode res;

struct Memory{
    char *memory;
    size_t size;
};

static size_t callback(void *data, size_t size, size_t n_data, void *userp){
    size_t realsize = size * n_data;
    struct Memory *mem = (struct Memory *)userp;

    char *new_memory = realloc(mem->memory, mem->size + realsize + 1);
    if (new_memory == NULL) {
        fprintf(stderr, "Memory reallocation failed in callback!\n");
        return Error_Memory_Allocation;
    }
    mem->memory = new_memory;

    memcpy(&(mem->memory[mem->size]), data, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

// Helper function to safely extract a real number from JSON object
double get_real_value(json_t *obj, const char *key){
    json_t *value = json_object_get(obj, key);
    return json_is_real(value) ? json_real_value(value) : 0.0;
}

int fetch_data(const char *api_key, const char *city, const char *units, struct WeatherInfo *data, double temperature_h, double temperature_l, int  humidity_h, int wind_speedh){
    char api_url[256];
    struct Memory chunk = {NULL, 0};

    f_handler = curl_easy_init();
    if (f_handler == NULL) {
        fprintf(stderr, "HTTP REQUEST FAILED\n");
        return -1;
    }

    snprintf(api_url, sizeof(api_url), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=%s", city, api_key, units);
    curl_easy_setopt(f_handler, CURLOPT_URL, api_url);
    curl_easy_setopt(f_handler, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(f_handler, CURLOPT_WRITEDATA, &chunk);

    // Printing detailed output
    curl_easy_setopt(f_handler, CURLOPT_VERBOSE, 1L); // 1L enables verbose mode
    res = curl_easy_perform(f_handler);
    if (res != CURLE_OK) {
        fprintf(stderr, "ERROR!! %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        return -1;
    }

    long server_response;
    curl_easy_getinfo(f_handler, CURLINFO_RESPONSE_CODE, &server_response);
    if (res == CURLE_OK) {
        switch (server_response) {
            case 200:
                printf("\nDATA RETRIEVED SUCCESSFULLY\n!");

                FILE *file1 = fopen("file1.json", "a"); // raw data file
                if (file1 == NULL) {
                    fprintf(stderr, "ERROR IN OPENING FILE WITH RAW DATA\n");
                    free(chunk.memory);
                    return Error_File_Opening;
                } else {
                    fwrite(chunk.memory, 1, chunk.size, file1);
                    fclose(file1);
                }

                json_error_t err_response;
                json_t *json_parsed = json_loads(chunk.memory, 0, &err_response);

                if (!json_parsed) {
                    fprintf(stderr, "JSON parsing error at line %d, column %d: %s\n", err_response.line, err_response.column, err_response.text);
                    free(chunk.memory);
                    curl_easy_cleanup(f_handler);
                    return Error_JSON_Parsing;
                } else {
                    const char *main_work = "main";
                    json_t *main = json_object_get(json_parsed, main_work);

                    double temperature = get_real_value(main, "temp");
                    double feel = get_real_value(main, "feels_like");
                    int pressure = json_integer_value(json_object_get(main, "pressure"));
                    int humidity = json_integer_value(json_object_get(main, "humidity"));

                    if (temperature > temperature_high) {
                        printf("\n Scorching heat alert! Temperature soaring above %.2f °C \n", temperature);
                    } else if (temperature < temperature_low) {
                        printf("\n❄️ Brrr! Temperature dipping below %.2f °C ❄️\n", temperature);
                    } else if (humidity > humidity_high) {
                        printf("\nSticky situation! Humidity creeping up to %d%% \n", humidity);
                    } else {
                        printf("\nWeather conditions are holding steady!\n");
                    }

                    json_t *my_array = json_object_get(json_parsed, "weather");
                    json_t *weather_des = json_array_get(my_array, 0); // Assuming only one weather element
                    const char *description = json_string_value(json_object_get(weather_des, "description"));

                    time_t current = time(NULL);
                    char note_time[100];
                    strftime(note_time, sizeof(note_time), "[%Y-%m-%d %H:%M:%S]", localtime(&current));

                    FILE *file2 = fopen("file2.json", "a"); // processed data file
                    if (file2 != NULL) {
                        fprintf(file2, "---------------------------\n");
                        fprintf(file2, "Weather Report for %s \nTime %s:", city, note_time);
                        fprintf(file2, "\n---------------------------\n");

                        fprintf(file2, "TEMPERATURE : %.2f °C\n", temperature);
                        fprintf(file2, "FEELS LIKE : %.2f °C\n", feel);
                        fprintf(file2, "PRESSURE : %d mb/hPa\n", pressure);
                        fprintf(file2, "HUMIDITY : %d \n", humidity);
                        fprintf(file2, "DESCRIPTION: %s\n", description);
                        
                        printf("\n---------------------------\n");
    			printf("Weather Report for %s \nTime %s:", city, note_time);
    			printf("\n---------------------------\n");

    			printf("TEMPERATURE : %.2f °C\n", temperature);
    			printf("FEELS LIKE : %.2f °C\n", feel);
     			printf("PRESSURE : %d mb/hPa\n", pressure);
    			printf("HUMIDITY : %d \n", humidity);
    			printf("DESCRIPTION: %s\n", description);

                        fclose(file2);
                    } else {
                        fprintf(stderr, "ERROR OPENING FILE FOR PROCESSED DATA\n");
                    }
		    append_data(data, temperature, humidity, pressure);  
                    json_decref(json_parsed);
                }
                break;
            case 401:
                printf("\nUNAUTHORIZED ACCESS!\n");
                break;
            case 403:
                printf("\nACCESS FORBIDDEN!\n");
                break;
            case 404:
                printf("\nNOT FOUND!\n");
                break;
            default:
                printf("\nUNEXPECTED API ERROR!\n");
        }
        free(chunk.memory);
        curl_easy_cleanup(f_handler);
    } else {
        printf("\nERROR IN RETRIEVING RESPONSE!\n\t\t>_<\n");
    }

    return res;
}

void set_data(struct WeatherInfo *data){
    memset(data, 0, sizeof(*data));
    data->total_count = 0;
}

void append_data(struct WeatherInfo *data, double temperature, double humidity, double pressure) {
    if (data->total_count < max_data){
        data->temperature[data->total_count] = temperature;
        data->humidity[data->total_count] = humidity;
        data->pressure[data->total_count] = pressure;

        data->total_count++;
    } else {
        set_data(data);
        data->temperature[data->total_count] = temperature;
        data->humidity[data->total_count] = humidity;
        data->pressure[data->total_count] = pressure;

        data->total_count++;
    }
}

double calc_avg(const double *arr, int max_size){
    if (max_size <= 0) {
        return 0.0;
    } else {
        double sum = 0.0;
        int i = 0;
        while (i < max_size) {
            sum += arr[i];
            ++i;
        }
        return sum / max_size;
    }
}

int calc_avg_temp(const struct WeatherInfo *data){
    return round(calc_avg(data->temperature, data->total_count));
}

int calc_avg_press(const struct WeatherInfo *data){
    return round(calc_avg(data->pressure, data->total_count));
}

int calc_avg_hum(const struct WeatherInfo *data){
    return round(calc_avg(data->humidity, data->total_count));
}

void reset_files(){
    FILE *file1 = fopen("file1.json", "w"); 
    if (file1 != NULL) {
        fclose(file1);
    } else {
        fprintf(stderr, "ERROR OPENING FILE WITH RAW DATA FOR CLEARING\n");
    }

    FILE *file2 = fopen("file2.json", "w");
    if (file2 != NULL) {
        fclose(file2);
    } else {
        fprintf(stderr, "ERROR OPENING FILE WITH PROCESSED DATA FOR CLEARING\n");
    }
}

void set_run_time() {
    FILE* run_file = fopen(runtime_file, "w");
    if (run_file != NULL) {
        fprintf(run_file, "%d", run_time);
        fclose(run_file);
    } else {
        fprintf(stderr, "ERROR OPENING RUN TIME FILE FOR WRITING\n");
    }
}

void get_run_time() {
    FILE* run_file = fopen(runtime_file, "r");
    if (run_file != NULL) {
        fscanf(run_file, "%d", &run_time);
        fclose(run_file);
    }
}
	
int main(){
    const char *api_key = "d4b08a1ffc2b2f2e690c696049e62cc5";
    const char *city = "karachi";
    const char *units = "metric";
    get_run_time();

    int max_runs=28;
    struct WeatherInfo final_week_data;
    set_data(&final_week_data);
    ++run_time;
    
    if (run_time<=max_runs){
    	int result = fetch_data(api_key, city, units, &final_week_data, temperature_high, temperature_low, humidity_high, wind_speed);
    	printf("run time:%d",run_time);
    	if (result != 0) {
        	if (result == Error_Memory_Allocation) {
             		fprintf(stderr, "Memory Allocation Failed!\n");
        	} else if (result == Error_JSON_Parsing) {
             		fprintf(stderr, "JSON Parsing Error!\n");
        	} else {
             		fprintf(stderr, "An error occurred during data fetching\n");
        	}
    	}
    	if (run_time == max_runs) {
            int avg_temp = calc_avg_temp(&final_week_data);
            int avg_press = calc_avg_press(&final_week_data);
            int avg_hum = calc_avg_hum(&final_week_data);

	    printf("\n**********************\n");
            printf("Weekly Analysis Report:");
            printf("\n**********************\n");
            printf("Average Temperature: %d °C\n", avg_temp);
            printf("Average Pressure: %d mb/hPa\n", avg_press);
            printf("Average Humidity: %d%%\n", avg_hum);
            
            printf("\nCLEARING FILES!");
            reset_files();
            set_data(&final_week_data);
            run_time=0;
       }
       set_run_time();
    }
    return 0;
}
