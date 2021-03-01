#ifndef AJAOne_h
#define AJAOne_h

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <nickstuff.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BleKeyboard.h>
#include <IRremote.h>
#include <TinyPICO.h>

#define AJAONE_VERSION "AJAOne Remote BT V1.2"
#define BOARDNAME "AJARemoteBT_"
#define WIFIPORTAL "AJAOneAP"

#define TIMES_TO_SEND 10U

#define dot_mqtttraffic 0x4C0099   // blue-purple
#define dot_booting 0xFFFF00       // yellow
#define dot_wifi 0x990099          // purple
#define dot_goodboot 0x00FF00      // green
#define dot_reset 0xFF0000         // red
#define dot_mqttconnect 0x0000FF   // blue

//mqtt credentials
char mqtt_server[20] = "";
char mqtt_port[6] = "1883";
char mqtt_user[15] = "";
char mqtt_password[15] = "";
bool noconfig = false;
bool isxmitting = false;

char HOME_ASSISTANT_MQTT_DISCOVERY_TOPIC[60];
char friendlyName[65];

int pressTime = 0;
char boardname[30];

int IR_RECEIVE_PIN = 15;
//int IR_RECEIVE_PIN = 34;

//Firmware revision
const char myversion[30] = AJAONE_VERSION; //"AJAOne V1.5";

bool busy;
byte mac[6];
long lastkeyMsg = 0;

#endif
