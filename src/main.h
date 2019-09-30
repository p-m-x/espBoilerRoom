#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <PubSubClient.h>
#include <WaterFlowMetricsSender.h>
#include <TemperaturesSender.h>
#include <OneWire.h>
#include <Ticker.h>


#define WATER_FLOW_METER_COLD_PIN  D2
#define WATER_FLOW_METER_HOT_PIN  D1
#define ONE_WIRE_BUS D4

Ticker ledTicker;

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

OneWire oneWire(ONE_WIRE_BUS);

char mqtt_server[40];
char mqtt_port[6] = "1883";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";

void saveConfigCallback ();
void ledTick();
void mqttClientReconnect();

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager);

// water flow meter
void ICACHE_RAM_ATTR waterMeterColdISR();
void ICACHE_RAM_ATTR waterMeterHotISR();
void setupWaterFlowMeter();

// temperature temperature sensors
void setupTemperatureSensors();

// configuration form file system
void loadConfigFromFS();

// get sensors data from sesor senders classes
void sensorsPublishMqttCallback(String topic, String payload);
