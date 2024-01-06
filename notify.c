#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>
#include <libnotify/notify.h>



#define API_ENDPOINT "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s"
#define Error_Memory_Allocation -2
#define Error_JSON_Parsing -3
#define Error_File_Opening -4

CURL *f_handler;
CURLcode res;

struct Memory{
    char *memory;
    size_t size;
    size_t memory_capacity;
};

static size_t callback(void *data, size_t size, size_t n_data, void *userp) {
   size_t realsize = size * n_data;
   struct Memory *mem = (struct Memory*)userp;

   if (mem->memory == NULL || mem->size == 0) { // Check for initial allocation
       mem->memory = malloc(realsize+1); // Initialize to an empty string
       if (mem->memory == NULL) {
           fprintf(stderr, "-----OOPS-----\nMemory allocation failed in callback\n!");
           return Error_Memory_Allocation;
       }
       mem->memory[0] = '\0';  // Null terminator
       mem->size = 0;
   }

   char *repeat = realloc(mem->memory, mem->size + realsize + 1);
   if (!repeat) {
       printf("NOT ENOUGH MEMORY!\n");
       free(mem->memory); // Free previously allocated memory on failure
       return 0;
   }

   mem->memory = repeat;
   memcpy(&(mem->memory[mem->size]), data, realsize);
   mem->size += realsize;
   mem->memory[mem->size] = '\0';

   return realsize;
}


int fetch_data(const char *api_key,const char *city,double temperature_high,double temperature_low,int humidity_high,int wind_speed){
    char api_url[256];
    struct Memory *chunk=NULL;
    json_t *json_parsed; //represent whole json parsed data

    chunk = malloc(sizeof(struct Memory));
    
    if (chunk == NULL) {
    printf("Memory allocation for chunk failed!\n");
    return -1;  // Or implement alternative error handling
    }
    chunk->memory = NULL;  // Initialize to NULL
    chunk->size = 0;
    chunk->memory_capacity = 0;  // Add a field for capacity
    
    f_handler = curl_easy_init();
    if (f_handler==NULL){
        fprintf(stderr,"ERROR!\nHTTP REQUEST FAILED\n");
        return -1;
    }

    snprintf(api_url, sizeof(api_url), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", city, api_key);
    curl_easy_setopt(f_handler,CURLOPT_URL,api_url);
    curl_easy_setopt(f_handler, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(f_handler, CURLOPT_WRITEDATA, &chunk);

    //printing detailed o/p
    curl_easy_setopt(f_handler, CURLOPT_VERBOSE, 1L); //1L enables verbose mode
    res = curl_easy_perform(f_handler);
    if (res!=CURLE_OK){
        fprintf(stderr,"ERROR!!%s\n",curl_easy_strerror(res));
        return -1;
    }
    long server_response;
    curl_easy_getinfo(f_handler,CURLINFO_RESPONSE_CODE,&server_response);
    if (res==CURLE_OK){
        switch (server_response){
            case 200:
                printf("\nDATA RETRIEVED SUCCESSFULLY\n!");

                FILE *file1=fopen("file1.json","w");                //raw data file
                if (file1==NULL){
                    fprintf(stderr,"\nERROR IN OPENING FILE WITH RAW DATA\n!!");
                    return Error_File_Opening;
                }else{
                    fwrite(chunk->memory,sizeof(char),chunk->size,file1);
                    fclose(file1);
                }
                json_error_t err_response;
                json_parsed=  json_loads(chunk->memory,0,&err_response);
                char err_handle[100];
                time_t current=time(0);
                strftime(err_handle,sizeof(err_handle),"[%Y-%m-%d %H:%M:%S] JSON error : ",localtime(&current));
                    
                if (!json_parsed){
                    fprintf(stderr,"\n---JSON parsing error at line %d, column %d :%s\n",err_response.line,err_response.column,err_response.text);
                    switch (err_response.text[0]){
                        case 'U':
                            fprintf(stderr,"\nUnexpected Token!\nCheck json syntax and data format >_<");
                            break;
                        case 'M':
                            fprintf(stderr,"\nMemory Allocation Failed!\nCheck system resources >_<");
                            break;
                        case 'I':
                            fprintf(stderr,"\nInvalid Value!\n Check numericals and string formats >_<");
                            break;
                        default:
                            fprintf(stderr,"\nRefer to API documentation for further details.\n");
                        }
                }else if (json_parsed){
                    const char*main_work="main";
                    json_t *main=json_object_get(json_parsed,main_work);
                
                    double temperature=json_number_value(json_object_get(main,"temp"));
                    double feel=json_number_value(json_object_get(main,"feels_like"));
                    int pressure=json_integer_value(json_object_get(main,"pressure"));
                    int humidity=json_integer_value(json_object_get(main,"humidity"));
                    
                    if (temperature > temperature_high) {
                printf("\n Scorching heat alert! Temperature soaring above %.2f °C \n", temperature);
            } else if (temperature < temperature_low) {
                printf("\n❄️ Brrr! Temperature dipping below %.2f °C ❄️\n", temperature);
            }else if (humidity > humidity_high) {
                printf("\nSticky situation! Humidity creeping up to %d%% \n", humidity);
            }notify_init("Weather Alert");
            NotifyNotification *notification = notify_notification_new("Brrr! It's cold!", "Temperature dipping below %.2f °C", temperature);
            notify_notification_show(notification, NULL);
            g_object_unref(G_OBJECT(notification));
        } else if (humidity > humidity_high) {
            // Send desktop notification
            notify_init("Weather Alert");
            NotifyNotification *notification = notify_notification_new("Sticky situation!", "Humidity creeping up to %d%%", humidity);
            notify_notification_show(notification, NULL);
            g_object_unref(G_OBJECT(notification));
        } else {
            printf("\nWeather conditions are holding steady!\n");
                    json_t *my_array = json_object_get(json_parsed, "weather");
                    json_t *weather_des = json_array_get(my_array,0);  // Assuming only one weather element
                    const char *description = json_string_value(json_object_get(weather_des, "description"));
                    
                    
		      time_t current = time(NULL); 
		      char note_time[100];
		      strftime(note_time, sizeof(note_time), "%Y-%m-%d %H:%M:%S", localtime(&current));
                    FILE *file2=fopen("file2.json","w");         //processed data file
                    if (file2!=NULL){
                        fprintf(file2,"---------------------------\n");
                        fprintf(file2,"Weather Report for %s \nTime %s:",city,note_time);
                        fprintf(file2,"---------------------------\n");

                        fprintf(file2,"TEMPERATURE : %.2f °C\n",temperature);
                        fprintf(file2,"FEELS LIKE : %.2f °C\n",temperature);
                        fprintf(file2,"PRESSURE : %d mb/hPa\n",pressure);
                        fprintf(file2,"HUMIDITY : %d \n",humidity);
                        fprintf(file2, "DESCRIPTION: %s\n", description);

                        fclose(file2);
                    }else{
                        fprintf(stderr,"\nERROR OPENING FILE FOR PROCESSED DATA\n");
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
    }else{
        printf("\nERROR IN RETRIEVING RESPONSE!\n\t\t>_<\n");
    }
    free(chunk->memory);
    curl_easy_cleanup(f_handler);
    
    return res;
}


int main() {

    const char *api_key = "d4b08a1ffc2b2f2e690c696049e62cc5";
    const char *city = "karachi";

    double temperature_high = 60.0;  
    double temperature_low = 00.0;   
    int humidity_high = 80;          
    int wind_speed= 25;             
    int result = fetch_data(api_key, city,temperature_high,temperature_low,humidity_high,wind_speed);  

    if (result != 0) {
        if (result == Error_Memory_Allocation) {
            fprintf(stderr, "\nMemory Allocation Failed!\n");
        } else if (result == Error_JSON_Parsing) {
            fprintf(stderr, "\nJSON Parsing Error!\n");
        } else {
            fprintf(stderr, "\nAn error occurred during data fetching\n");
        }
    }
    return 0;
}
