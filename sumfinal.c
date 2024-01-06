#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>

#define API_ENDPOINT "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s"
#define Error_Memory_Allocation -2
#define Error_JSON_Parsing -3
#define Error_File_Opening -4

CURL *f_handler;
CURLcode res;

struct Memory {
    char *memory;
    size_t size;
};

static size_t callback(void *data, size_t size, size_t n_data, void *userp) {
    size_t realsize = size * n_data;
    struct Memory *mem = (struct Memory *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        fprintf(stderr, "Memory reallocation failed in callback!\n");
        return Error_Memory_Allocation;
    }

    memcpy(&(mem->memory[mem->size]), data, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

// Helper function to safely extract a real number from JSON object
double get_real_value(json_t *obj, const char *key) {
    json_t *value = json_object_get(obj, key);
    return json_is_real(value) ? json_real_value(value) : 0.0;
}

int fetch_data(const char *api_key, const char *city,const char *units, double temperature_high, double temperature_low, int humidity_high, int wind_speed) {
    char api_url[256];
    struct Memory chunk = {NULL, 0};

    f_handler = curl_easy_init();
    if (f_handler == NULL) {
        fprintf(stderr, "HTTP REQUEST FAILED\n");
        return -1;
    }

    snprintf(api_url, sizeof(api_url), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=%s", city, api_key,units);
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

                FILE *file1 = fopen("file1.json", "w"); // raw data file
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

                    FILE *file2 = fopen("file2.json", "w"); // processed data file
                    if (file2 != NULL) {
                        fprintf(file2, "---------------------------\n");
                        fprintf(file2, "Weather Report for %s \nTime %s:", city, note_time);
                        fprintf(file2, "\n---------------------------\n");

                        fprintf(file2, "TEMPERATURE : %.2f °C\n", temperature);
                        fprintf(file2, "FEELS LIKE : %.2f °C\n", feel);
                        fprintf(file2, "PRESSURE : %d mb/hPa\n", pressure);
                        fprintf(file2, "HUMIDITY : %d \n", humidity);
                        fprintf(file2, "DESCRIPTION: %s\n", description);

                        fclose(file2);
                    } else {
                        fprintf(stderr, "ERROR OPENING FILE FOR PROCESSED DATA\n");
                    }

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

int main() {
    const char *api_key = "d4b08a1ffc2b2f2e690c696049e62cc5";
    const char *city = "karachi";
    const char *units = "metric";

    double temperature_high = 60.0;
    double temperature_low = 0.0;
    int humidity_high = 80;
    int wind_speed = 25;
    int result = fetch_data(api_key, city,units, temperature_high, temperature_low, humidity_high, wind_speed);

    if (result != 0) {
        if (result == Error_Memory_Allocation) {
            fprintf(stderr, "Memory Allocation Failed!\n");
        } else if (result == Error_JSON_Parsing) {
            fprintf(stderr, "JSON Parsing Error!\n");
        } else {
            fprintf(stderr, "An error occurred during data fetching\n");
        }
    }
    return 0;
}
