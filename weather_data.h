#define weather_data_h
#define max_data 28

struct WeatherInfo {
    double temperature[max_data];
    double humidity[max_data];
    double pressure[max_data];
    int total_count;
};

void set_data(struct WeatherInfo *data);
void append_data(struct WeatherInfo *data, double temperature, double humidity, double pressure);
int calc_avg_temp(const struct WeatherInfo *data);
int calc_avg_press(const struct WeatherInfo *data);
int calc_avg_hum(const struct WeatherInfo *data);

