#include "mbed.h"
#include "i2cThread.h"

enum Opcode {
    OP_measure
};

struct Message {
    enum Opcode opcode;
    uint32 operand;
};

static Queue<Message, 32> queue;
static MemoryPool<Message, 16> pool;

void i2cTask(void) {
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_measure:
                    break;
            }
            pool.free(message);
        }
    }
}