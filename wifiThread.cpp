#include "mbed.h"
#include "wifiThread.h"
#include "https_request.h"

/*
 * The header file "secret.h" must contain following MACROs.
 * This file is intended to be ignored from the Git repository.
 *
 *     WIFI_SSID: SSID name of WiFi access point.
 *     WIFI_PASSWORD: Password of the access point.
 *     SERVER_DOMAIN: Domain name of the database server.
 *     SERVER_CGI: Path to the CGI to GET a request.
 *     SENSOR_ID: ID name of the measured sensor.
 */
#include "secret.h"

/* List of trusted root CA certificates
 *   IdenTrust TrustID X3 Root
 *
 * To add more root certificates, just concatenate them.
 */
const char SSL_CA_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
    "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n"
    "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n"
    "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n"
    "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n"
    "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n"
    "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n"
    "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n"
    "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n"
    "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n"
    "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n"
    "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n"
    "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n"
    "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n"
    "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n"
    "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n"
    "-----END CERTIFICATE-----\n"
    ;

enum Opcode {
    OP_temperature,
    OP_pressure
};

struct Message {
    enum Opcode opcode;
    double operand;
};

static Queue<Message, 32> queue;
static MemoryPool<Message, 32> pool;

void sendTemperature(double temperature) {
    Message *message = pool.alloc();
    if (message) {
        message->opcode = OP_temperature;
        message->operand = temperature;
        queue.put(message);
    }
}

void sendPressure(double pressure) {
    Message *message = pool.alloc();
    if (message) {
        message->opcode = OP_pressure;
        message->operand = pressure;
        queue.put(message);
    }
}

WiFiInterface *wifi = 0;

static void wifiInit(void) {
    nsapi_error_t errorCode;

    wifi = WiFiInterface::get_default_instance();
    printf("Connecting: %08X\n", (uint32)wifi);
    for (;;) {
        errorCode = wifi->connect(WIFI_SSID, WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
        if (errorCode == NSAPI_ERROR_OK) break;
        printf("ERROR=%08X\n", errorCode);
        ThisThread::sleep_for(2000); // If for some reason it doesnt work wait 2s and try again
    }
    printf("Connected\n");
}

void sendData(Message *message, string suffix) {
    char url[128];
    sprintf(url, "https://%s/%s?ID=%s_%s&PRESSURE=%f",
        SERVER_DOMAIN, SERVER_CGI, SENSOR_ID, suffix.c_str(), message->operand);
    HttpsRequest* request = new HttpsRequest(wifi, SSL_CA_PEM, HTTP_GET, url);
    HttpResponse* response = request->send();
    if (response) {
        printf("SENT: %s=%.2f\n", suffix.c_str(), message->operand);
    } else {
        printf("HttpRequest failed (error code %d)\n", request->get_error());
    }
    delete request;
}

void wifiTask(void) {
    wifiInit();

    for (;;) {
        osEvent event = queue.get();
        if (event.status == osEventMessage) {
            Message *message = (Message *)event.value.p;
            switch (message->opcode) {
                case OP_temperature:
                    sendData(message, "T");
                    break;
                case OP_pressure: 
                    sendData(message, "P");
                    break;
            }
            pool.free(message);
        }
    }
}
