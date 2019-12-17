#include "mbed.h"
#include "ledThread.h"
#include "i2cThread.h"
#include "calculationThread.h"
#include "wifiThread.h"

static const int32 SEND_PERIOD = 10;
static int32 measureCount = 0;

Thread ledThread;
Thread i2cThread;
Thread calculationThread;
Thread wifiThread;

// main() runs in its own thread in the OS
int main() {
    printf("Started.\n");

    ledThread.start(ledTask);
    i2cThread.start(i2cTask);
    calculationThread.start(calculationTask);
    wifiThread.start(wifiTask);
    for (;;) {
        if (--measureCount <= 0) {
            setSendRequest();
            measureCount = SEND_PERIOD;
        }
        sensorMeasure();
        setLedRed(true);
        ThisThread::sleep_for(100);
        setLedRed(false);
        ThisThread::sleep_for(2900);
    }
}

