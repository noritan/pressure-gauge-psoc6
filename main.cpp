#include "mbed.h"
#include "ledThread.h"

Thread ledThread;

// main() runs in its own thread in the OS
int main() {
    printf("Started.\n");

    ledThread.start(ledTask);
    for (;;) {
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

