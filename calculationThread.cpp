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

struct Coe {
    uint8 b00[2];  // A0
    uint8 bt1[2];
    uint8 bt2[2];
    uint8 bp1[2];
    uint8 b11[2];  // A8
    uint8 bp2[2];
    uint8 b12[2];
    uint8 b21[2];
    uint8 bp3[2];  // B0
    uint8 a0[2];
    uint8 a1[2];
    uint8 a2[2];
    uint8 b00_a0_ex;  // B8
};

void setSensorCoe(char *coe) {
    Message *message = pool.alloc();
    message->opcode = OP_setCoe;
    memcpy(message->operand, coe, sizeof(Coe));
    queue.put(message);
}

struct Rawdata {
    uint8 dp[3];  // pressure
    uint8 dt[3];  // temperature
};

void setSensorRawData(char *rawdata) {
    Message *message = pool.alloc();
    message->opcode = OP_setRawData;
    memcpy(message->operand, rawdata, sizeof(Rawdata));
    queue.put(message);

}

void calculationTask(void) {
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_setCoe:
                    printf("COE=");
                    for (uint32 i = 0; i < sizeof(Coe); i++) {
                        printf("%02X ", message->operand[i]);
                    }
                    printf("\n");
                    break;
                case OP_setRawData:
                    printf("RAW=");
                    for (uint32 i = 0; i < sizeof(Rawdata); i++) {
                        printf("%02X ", message->operand[i]);
                    }
                    printf("\n");
                    break;
            }
            pool.free(message);
        }
    }
}