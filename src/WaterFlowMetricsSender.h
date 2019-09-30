#include <Arduino.h>
#include <FlowMeter.h>

#define BASE_INTERVAL_MS 60000
#define ACTIVE_INTERVAL_MS 1000

#define MQTT_TOPIC_WATER_VOLUME_TPL "sensors/boiler-room/%s-water/volume" 
#define MQTT_TOPIC_WATER_TOTAL_VOLUME_TPL "sensors/boiler-room/%s-water/total-volume"
#define MQTT_TOPIC_WATER_FLOW_RATE_TPL "sensors/boiler-room/%s-water/flow-rate" 

class WaterFlowMetricsSender {

    public:
        WaterFlowMetricsSender(unsigned int pin, const char *prefix, std::function<void(String topic, String payload)> sendCallbackFunction);
        void begin(void);
        void loop(void);
        void count(void);

    private:
        std::function<void(String topic, String payload)> _sendCallbackFunction;
        FlowMeter _flowMeter;
        const char *_prefix;
        unsigned int _pin;
        unsigned long _sendTime = 0;
        unsigned long _tickTime = 0;
        double _lastWaterCurrentVolume = 0;
        unsigned int _interval = BASE_INTERVAL_MS;
        void sendData(void);
        String getTopic(const char *tpl);

};