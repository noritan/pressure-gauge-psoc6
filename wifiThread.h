#ifndef WIFI_THREAD_H
#define WIFI_THREAD_H

void wifiTask(void);
void sendTemperature(double temperature);
void sendPressure(double pressure);

#endif // !defined(WIFI_THREAD_H)