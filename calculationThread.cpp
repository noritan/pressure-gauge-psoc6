#include "mbed.h"
#include "calculationThread.h"

enum Opcode {
    OP_setCoe,
    OP_setRawData
};

struct Message {
    enum Opcode opcode;
    char operand[25];
};

static Queue<Message, 32> queue;
static MemoryPool<Message, 16> pool;

void calculationTask(void) {
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_setCoe:
                    break;
                case OP_setRawData:
                    break;
            }
            pool.free(message);
        }
    }
}