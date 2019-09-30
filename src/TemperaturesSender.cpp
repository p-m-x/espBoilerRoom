#include <TemperaturesSender.h>


TemperaturesSender::TemperaturesSender(OneWire *oneWire, std::function<void(String topic, String payload)> sendCallbackFunction) {
    _oneWire = oneWire;
    _sendCallbackFunction = sendCallbackFunction;
}

void TemperaturesSender::begin(void) {
    _temperatures = DallasTemperature(_oneWire);
    _oneWire->reset();
    _oneWire->reset_search();
    Serial.print(F("Searching for temperature sensors... "));
    _temperatures.begin();
    _sensorsCount = _temperatures.getDS18Count();
    Serial.print(F("found "));
    Serial.print(_sensorsCount);
    Serial.println(F(" devices"));
}

void TemperaturesSender::setIntervalMs(unsigned int interval) {
    _interval = interval;
}

String TemperaturesSender::getTopic(uint8_t sensorAddress[8]) {
    String topic = TEMP_MQTT_TOPIC_TPL;
    for (uint8_t i = 0; i < sizeof(sensorAddress); i++) {
        topic += String(sensorAddress[i], HEX);
    }
    return topic;
}

void TemperaturesSender::loop(void) {
    if (_sensorsCount == 0) {
        return;
    }

    if (_conversionTime == 0 || millis() - _conversionTime >= TEMP_CONVERSION_INTERVAL_MS) {
        _conversionTime = millis();
        _temperatures.requestTemperatures();
    }

    if (_sendTime == 0 || millis() - _sendTime >= _interval) {
        _sendTime = millis();
        if (_temperatures.isConversionComplete()) {
        uint8_t address[8];
        for (unsigned int i = 0; i < _sensorsCount; i++) {
          _temperatures.getAddress(address, i);
          if (_temperatures.isConnected(address)) {
            _sendCallbackFunction(getTopic(address).c_str(), String(_temperatures.getTempC(address), 2).c_str());
          } else {
            Serial.print(F("ERROR: Device #"));
            Serial.print(i);
            Serial.println(F(" not connected"));
          }          
        }
      } else {
        Serial.println(F("ERROR: temperature conversion not completed"));
      } 
    }
}