#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BASE_INTERVAL_MS 60000
#define TEMP_CONVERSION_INTERVAL_MS 60000
#define TEMP_MQTT_TOPIC_TPL "sensors/boiler-room/temperature/"

class TemperaturesSender {
    public:
        TemperaturesSender(OneWire *oneWire, std::function<void(String topic, String payload)> sendCallbackFunction);
        void begin(void);
        void loop(void);
        void setIntervalMs(unsigned int interval);

    private:
        OneWire *_oneWire;
        DallasTemperature _temperatures;
        std::function<void(String topic, String payload)> _sendCallbackFunction;
        unsigned int _sensorsCount = 0;
        unsigned long _sendTime = 0;
        unsigned long _conversionTime = 0;
        unsigned int _interval = BASE_INTERVAL_MS;
        String getTopic(uint8_t sensorAddress[8]);
};