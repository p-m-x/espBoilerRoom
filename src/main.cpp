#include <main.h>

#define DATA_SEND_INTERVAL_MS 30000
#define TEMP_SENSOR_CALCULATION_INTERVAL_MS 1000
#define DS_SENSORS_MAX_COUNT 20

WaterFlowMetricsSender waterFlowMeterSenderCold = WaterFlowMetricsSender(WATER_FLOW_METER_COLD_PIN, "cold", sensorsPublishMqttCallback);
WaterFlowMetricsSender waterFlowMeterSenderHot = WaterFlowMetricsSender(WATER_FLOW_METER_HOT_PIN, "hot", sensorsPublishMqttCallback);

TemperaturesSender temperaturesSender(&oneWire, sensorsPublishMqttCallback);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  

  //set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ledTicker.attach(0.6, ledTick);

  loadConfigFromFS();  

  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_blynk_token);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);


  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("Saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  // module initializations
  ledTicker.detach();
  //keep LED on
  digitalWrite(LED_BUILTIN, LOW);

  mqttClient.setServer(mqtt_server, atoi(mqtt_port));

  setupWaterFlowMeter();
  setupTemperatureSensors();
  
}

void loop() {
  if (!WiFi.isConnected()) {
    ESP.restart();
  }

  if (!mqttClient.connected()) {
    mqttClientReconnect();
  }
  mqttClient.loop();

  if (mqttClient.connected()) {
    // sending temperatures
    temperaturesSender.loop();
    // sending water flow metrics   
    waterFlowMeterSenderCold.loop();
    waterFlowMeterSenderHot.loop();    
  }
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void ledTick() {
  //toggle state
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

void setupWaterFlowMeter() {
  waterFlowMeterSenderCold.begin();
  waterFlowMeterSenderHot.begin();
  attachInterrupt(WATER_FLOW_METER_COLD_PIN, waterMeterColdISR, RISING);
  attachInterrupt(WATER_FLOW_METER_HOT_PIN, waterMeterHotISR, RISING);
}

void waterMeterColdISR() {
  waterFlowMeterSenderCold.count();
}

void waterMeterHotISR() {
  waterFlowMeterSenderHot.count();
}

void setupTemperatureSensors() {
  temperaturesSender.begin();
}

void sensorsPublishMqttCallback(String topic, String payload) {
  digitalWrite(LED_BUILTIN, LOW);
  mqttClient.publish(topic.c_str(), payload.c_str());
  digitalWrite(LED_BUILTIN, HIGH);
}

void loadConfigFromFS() {
  Serial.print(F("Mounting FS... ")); 
  if (SPIFFS.begin()) {
    Serial.println(F("file system mounted"));
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.print(F("Opening config file... "));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.print(F("OK\nLoading content..."));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println(F("JSON content parsed"));

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println(F(" failed to load JSON config"));
        }
      } else {
        Serial.println(F("failed"));
      }
    }
  } else {
    Serial.println(F(" failed to mount FS"));
  }
}

void mqttClientReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection... ");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ledTicker.attach(0.2, ledTick);
}

