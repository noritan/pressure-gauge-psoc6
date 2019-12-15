#include "mbed.h"
#include "ledThread.h"

static DigitalOut led_r(LED1/*P0_3*/, 1);
static DigitalOut led_g(LED3/*P1_1*/, 1);
static DigitalOut led_b(LED2/*P11_1*/, 1);

enum Opcode {
    OP_setRed,
    OP_setGreen,
    OP_setBlue
};

struct Message {
    Opcode opcode;
    uint32 operand;
};

static Queue<Message, 32> queue;
static MemoryPool<Message, 16> pool;

void setLedRed(bool value) {
    Message *message = pool.alloc();
    if (message) {
        message->opcode = OP_setRed;
        message->operand = (value)?(0):(1);
        queue.put(message);
    }
}

void setLedGreen(bool value) {
    Message *message = pool.alloc();
    if (message) {
        message->opcode = OP_setGreen;
        message->operand = (value)?(0):(1);
        queue.put(message);
    }
}

void setLedBlue(bool value) {
    Message *message = pool.alloc();
    if (message) {
        message->opcode = OP_setBlue;
        message->operand = (value)?(0):(1);
        queue.put(message);
    }
}

void ledTask(void) {
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_setRed:
                    led_r = message->operand;
                    break;
                case OP_setGreen:
                    led_g = message->operand;
                    break;
                case OP_setBlue:
                    led_b = message->operand;
                    break;
            }
            pool.free(message);
        }
    }
}