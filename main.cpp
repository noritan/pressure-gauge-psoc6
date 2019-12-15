#include "mbed.h"
#include "ledThread.h"
#include "i2cThread.h"

Thread ledThread;
Thread i2cThread;

// main() runs in its own thread in the OS
int main() {
    printf("Started.\n");

    ledThread.start(ledTask);
    i2cThread.start(i2cTask);
    for (;;) {
        sensorMeasure();
        setLedRed(true);
        ThisThread::sleep_for(100);
        setLedRed(false);
        ThisThread::sleep_for(900);
        setLedGreen(true);
        ThisThread::sleep_for(100);
        setLedGreen(false);
        ThisThread::sleep_for(900);
        setLedBlue(true);
        ThisThread::sleep_for(100);
        setLedBlue(false);
        ThisThread::sleep_for(900);
    }
}

