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

struct Context {
    double temperature;
    double pressure;
    double b00;
    double bt1;
    double bt2;
    double bp1;
    double b11;
    double bp2;
    double b12;
    double b21;
    double bp3;
    double a0;
    double a1;
    double a2;
    double dt;
    double dp;
    double tr;
    double pr;
} context;

static bool sendRequestFlag = false;

void setSendRequest(void) {
    sendRequestFlag = true;
}

static void calculateK(uint8 *coe, double a, double s, double *result) {
    int32 x = (coe[0] << 8) | coe[1];
    if (x & 0x8000) { x -= 0x10000u; } // Sign extend
    *result = a + (s * x) / 32768.0;
}

void calculationTask(void) {
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_setCoe:
                    {
                        Coe *coe = (Coe *)message->operand;
                        int32 x;
                        
                        x = (coe->a0[0] << 12) | (coe->a0[1] << 4) | (coe->b00_a0_ex & 0x0F);
                        if (x & 0x80000) { x -= 0x100000u; } // Sign extend
                        context.a0 = (double)x / 16.0;
                        
                        calculateK(coe->a1, -6.3e-03,  4.3e-04, &context.a1);
                        calculateK(coe->a2, -1.9e-11,  1.2e-10, &context.a2);

                        x = (coe->b00[0] << 12) | (coe->b00[1] << 4) | ((coe->b00_a0_ex & 0xF0) >> 4);
                        if (x & 0x80000) { x -= 0x100000u; } // Sign extend
                        context.b00 = (double)x / 16.0;

                        calculateK(coe->bt1,  1.0e-01,  9.1e-02, &context.bt1);
                        calculateK(coe->bt2,  1.2e-08,  1.2e-06, &context.bt2);
                        calculateK(coe->bp1,  3.3e-02,  1.9e-02, &context.bp1);
                        calculateK(coe->b11,  2.1e-07,  1.4e-07, &context.b11);
                        calculateK(coe->bp2, -6.3e-10,  3.5e-10, &context.bp2);
                        calculateK(coe->b12,  2.9e-13,  7.6e-13, &context.b12);
                        calculateK(coe->b21,  2.1e-15,  1.2e-14, &context.b21);
                        calculateK(coe->bp3,  1.3e-16,  7.9e-17, &context.bp3);
                    }
                    printf("A0=%9.2e A1=%9.2e A2=%9.2e\n", context.a0, context.a1, context.a2);
                    printf("B00=%9.2e BP1=%9.2e BP2=%9.2e BP3=%9.2e\n", context.b00, context.bp1, context.bp2, context.bp3);
                    printf("BT1=%9.2e B11=%9.2e B21=%9.2e\n", context.bt1, context.b11, context.b21);
                    printf("BT2=%9.2e B12=%9.2e\n", context.bt2, context.b12);
                    break;
                case OP_setRawData:
                    {
                        Rawdata *rawdata = (Rawdata *)message->operand;
                        int32 t = ((rawdata->dt[0] << 16) | (rawdata->dt[1] << 8) | (rawdata->dt[2] << 0)) - 0x800000;
                        int32 p = ((rawdata->dp[0] << 16) | (rawdata->dp[1] << 8) | (rawdata->dp[2] << 0)) - 0x800000;
                        context.dt = (double)t;
                        context.dp = (double)p;

                        // Calculate temperature
                        context.tr = (context.a2 * context.dt + context.a1) * context.dt + context.a0;
                        context.temperature = context.tr / 256.0;

                        // Calculate pressure
                        double x3 = context.bp3;
                        double x2 = context.b21 * context.tr + context.bp2;
                        double x1 = (context.b12 * context.tr + context.b11) * context.tr + context.bp1;
                        double x0 = (context.bt2 * context.tr + context.bt1) * context.tr + context.b00;
                        context.pr = ((x3 * context.dp + x2) * context.dp + x1) * context.dp + x0;
                        context.pressure = context.pr / 100.0;
                    }
                    printf("T=%.2f P=%.2f\n", context.temperature, context.pressure);
                    if (sendRequestFlag) {
                        sendRequestFlag = false;
                        printf("SEND: T=%.2f P=%.2f\n", context.temperature, context.pressure);
                    }
                    break;
            }
            pool.free(message);
        }
    }
}