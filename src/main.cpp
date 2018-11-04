#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS D5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int deviceCount = 0;
char buf[16];

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager  wifiManager(&server,&dns);

DeviceAddress *deviceAddressList;

void printAddress(DeviceAddress deviceAddress);
void sprintAddress(char buf[], DeviceAddress deviceAddress);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(9600);
  Serial.println("\n\nesp8266 Boiler Room Control\n");

  if (!wifiManager.autoConnect("Boiler Room Control System")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  sensors.begin();
  deviceCount = sensors.getDeviceCount();

  Serial.print("DS18B20 devices count: ");
  Serial.println(deviceCount);

  deviceAddressList = (DeviceAddress *)malloc(sizeof(DeviceAddress) * deviceCount);
  for (int i = 0; i < deviceCount; i++) {
    sensors.getAddress(deviceAddressList[i], i);
    sensors.setResolution(deviceAddressList[i], 12);
  }

  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404);
  });

  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(BUILTIN_LED, LOW);
    deviceCount = 0;
    AsyncResponseStream *response = request->beginResponseStream("text/prometheus; version=0.4");
    response->print("# HELP temperature_c Calculated temperature in centigrade\n");
    response->print("# TYPE temperature_c gauge\n");

    sensors.requestTemperatures();
    delay(1);

    for (int i = 0; i < sizeof(deviceAddressList); i++) {

      if (sensors.isConnected(deviceAddressList[i])) {
        sprintAddress(buf, deviceAddressList[i]);
        response->printf("temperature_c{address=\"%s\"} ", buf);
        response->print(sensors.getTempCByIndex(i));
        response->print("\n");
        deviceCount++;
      }
    }

    response->print("\n");

    response->print("# HELP temperature_sensor_device_count temperature sensors count\n");
    response->print("# TYPE temperature_sensor_device_count gauge\n");
    response->printf("temperature_sensor_device_count{} %d\n\n", deviceCount);

    response->print("# HELP wifi_rssi_dbm Received Signal Strength Indication, dBm\n");
    response->print("# TYPE wifi_rssi_dbm counter\n");
    response->printf("wifi_rssi_dbm{} %d\n\n", WiFi.RSSI());

    response->print("# HELP heap_free_bytes Free heap in bytes\n");
    response->print("# TYPE heap_free_bytes gauge\n");
    response->printf("heap_free_bytes{} %d\n\n", ESP.getFreeHeap());

    response->print("# HELP uptime_seconds Uptime in seconds\n");
    response->print("# TYPE uptime_seconds gauge\n");
    response->printf("uptime_seconds{} %f\n\n", ((double)millis())/1000.0);

    request->send(response);
    digitalWrite(BUILTIN_LED, HIGH);

  });


  server.begin();

}

void loop() {

}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void sprintAddress(char buf[], DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) {
      sprintf(buf + sizeof(char) * i * 2, "0%x", deviceAddress[i]);
    } else {
      sprintf(buf + sizeof(char) * i * 2, "%x", deviceAddress[i]);
    }
  }
}
