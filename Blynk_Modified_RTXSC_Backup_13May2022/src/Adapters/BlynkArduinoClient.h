/**
 * @file       BlynkParam.h
 * @author     Volodymyr Shymanskyy / Robotronix
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Jan 2015 / March 2022 / April 2022 Adding Serial
 * @brief
 *
 */

#ifndef BlynkArduinoClient_h
#define BlynkArduinoClient_h

#include <BlynkApiArduino.h>
#include <Blynk/BlynkDebug.h>
#include <Client.h>

#if defined(ESP8266) && !defined(BLYNK_NO_YIELD)
    #define YIELD_FIX() BLYNK_RUN_YIELD();
#else
    #define YIELD_FIX()
#endif

template <typename Client>
class BlynkArduinoClientGen
{
public:
    BlynkArduinoClientGen(Client& c)
        : client(NULL), domain(NULL), port(0), isConn(false)
    {
        setClient(&c);
    }

    BlynkArduinoClientGen()
        : client(NULL), domain(NULL), port(0), isConn(false)
    {}

    void setClient(Client* c) {
        client = c;
        client->setTimeout(BLYNK_TIMEOUT_MS);
    }

    void begin(IPAddress a, uint16_t p) {
        domain = NULL;
        port = p;
        addr = a;
    }

    void begin(const char* d, uint16_t p) {
        domain = d;
        port = p;
    }

    bool connect() {
        millis_time_t connectingTime = BlynkMillis();
        uint32_t timeout = BLYNK_TIMEOUT_MS*5;
        BLYNK_LOG2(BLYNK_F("[BLYNK_ARDUINO_CLIENT] Timeout is "), timeout);
        if (domain) {
            while(!isConn){
                int countdown = 30 - ((BlynkMillis()-connectingTime)/1000);
                if(countdown <= 0) countdown = 0;
                BLYNK_LOG2(BLYNK_F("[BLYNK_ARDUINO_CLIENT] Connecting elapse "), countdown);
                BLYNK_LOG4(BLYNK_F("[BLYNK_ARDUINO_CLIENT] Connecting to "), domain, ':', port);
                if(BlynkMillis()-connectingTime > timeout and !isConn){
                    BLYNK_LOG1("[BLYNK_ARDUINO_CLIENT] Network Loss Detected! Network is probably down!");
                    BlynkDelay(1000);
                    BLYNK_LOG1("[BLYNK_ARDUINO_CLIENT] This is taking too long! Restarting now");
                    millis_time_t restartCountdown = millis();
                    for(int i=0; i<5; i++){
                        BLYNK_LOG2(BLYNK_F("[BLYNK_ARDUINO_CLIENT] Restarting in "), 5 - ((BlynkMillis()-restartCountdown)/1000));
                        BlynkDelay(1000);
                    }
                    ESP.restart(); // added 14 March 2022 FURTHER MODIFIED 1 APRIL 2022
                }
                isConn = (1 == client->connect(domain, port));
                BLYNK_LOG2(BLYNK_F("[BLYNK_ARDUINO_CLIENT] isConn: "), isConn); // could be 0 if no network (down)
                BlynkDelay(1000);
            }
            return isConn;

        } else { //if (uint32_t(addr) != 0) {
            BLYNK_LOG_IP("Connecting to ", addr);
            isConn = (1 == client->connect(addr, port));
            return isConn;
        }
        return false;
    }

    void disconnect() { isConn = false; client->stop(); }

#ifdef BLYNK_ENC28J60_FIX
    size_t read(void* buf, size_t len) {
        while (client->available() < len) { BLYNK_RUN_YIELD(); }
        return client->read((uint8_t*)buf, len);
    }
#else
    size_t read(void* buf, size_t len) {
        size_t res = client->readBytes((char*)buf, len);
        YIELD_FIX();
        return res;
    }
#endif

#ifdef BLYNK_RETRY_SEND
    size_t write(const void* buf, size_t len) {
        size_t sent = 0;
        int retry = 0;
        while (sent < len && ++retry < 10) {
            size_t w = client->write((const uint8_t*)buf+sent, len-sent);
            if (w != 0 && w != -1) {
                sent += w;
            } else {
                BlynkDelay(50);
#if defined(BLYNK_DEBUG) && defined(BLYNK_PRINT)
                BLYNK_PRINT_TIME();
                BLYNK_PRINT.print(BLYNK_F("Retry "));
                BLYNK_PRINT.print(retry);
                BLYNK_PRINT.print(BLYNK_F(" send: "));
                BLYNK_PRINT.print(sent);
                BLYNK_PRINT.print('/');
                BLYNK_PRINT.println(len);
#endif
            }
        }
        return sent;
    }
#else
    size_t write(const void* buf, size_t len) {
        YIELD_FIX();
        size_t res = client->write((const uint8_t*)buf, len);
        YIELD_FIX();
        return res;
    }
#endif

    bool connected() { YIELD_FIX(); return isConn && client->connected(); }
    int available() {  YIELD_FIX(); return client->available(); }

protected:
    Client*     client;
    IPAddress   addr;
    const char* domain;
    uint16_t    port;
    bool        isConn;
};

typedef BlynkArduinoClientGen<Client> BlynkArduinoClient;

#endif
