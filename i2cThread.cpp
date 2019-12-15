#include "mbed.h"
#include "i2cThread.h"
#include "calculationThread.h"

static const uint8 SENSOR_ADDRESS = 0x56 << 1; // 8-bit I2C address
static const uint8 REG_COE = 0xA0; // COE register address
static const uint8 REG_COE_LENGTH = 0x19; // COE parameter length
static const uint8 REG_CTRL_MEAS = 0xF4; // CTRL_MEAS register address
static const uint8 CTRL_MEAS_FORCE = 0x6D; // Force command
static const uint8 REG_TXD = 0xF7; // TXD register address
static const uint8 REG_TXD_LENGTH = 6; // TXD register length

static I2C i2c(P6_1, P6_0);

enum Opcode {
    OP_measure
};

struct Message {
    enum Opcode opcode;
    uint32 operand;
};

static Queue<Message, 32> queue;
static MemoryPool<Message, 16> pool;

void sensorMeasure(void) {
    Message *message = pool.alloc();
    message->opcode = OP_measure;
    message->operand = 0;
    queue.put(message);
}

void i2cTask(void) {
    {
        char coe[REG_COE_LENGTH];
        char cmd[1];
        cmd[0] = REG_COE;
        i2c.write(SENSOR_ADDRESS, cmd, 1);
        i2c.read(SENSOR_ADDRESS, coe, REG_COE_LENGTH);
        setSensorCoe(coe);
    }
    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_measure:
                    {
                        char cmd[2];
                        cmd[0] = REG_CTRL_MEAS;
                        cmd[1] = CTRL_MEAS_FORCE;
                        i2c.write(SENSOR_ADDRESS, cmd, 2);
                    }
                    ThisThread::sleep_for(1000);
                    {
                        char cmd[1];
                        char txd[REG_TXD_LENGTH];
                        cmd[0] = REG_TXD;
                        i2c.write(SENSOR_ADDRESS, cmd, 2);
                        i2c.read(SENSOR_ADDRESS, txd, REG_TXD_LENGTH);
                        setSensorRawData(txd);
                    }
                    break;
            }
            pool.free(message);
        }
    }
}