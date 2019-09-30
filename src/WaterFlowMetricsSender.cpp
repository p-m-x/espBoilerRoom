#include <WaterFlowMetricsSender.h>

WaterFlowMetricsSender::WaterFlowMetricsSender(unsigned int pin, const char *prefix, std::function<void(String topic, String payload)> sendCallbackFunction) {
    _pin = pin;
    _prefix = prefix;
    _sendCallbackFunction = sendCallbackFunction;
    pinMode(pin, INPUT_PULLUP);
}

void WaterFlowMetricsSender::begin(void) {
    _flowMeter = FlowMeter(_pin, FS400A);
    _flowMeter.reset();
}

void WaterFlowMetricsSender::count(void) {
    _flowMeter.count();
}

String WaterFlowMetricsSender::getTopic(const char *tpl) {
    char topic[100];
    sprintf(topic, tpl, _prefix);
    return String(topic);
}

void WaterFlowMetricsSender::loop(void) {
    
    if (_sendTime == 0 || millis() - _sendTime >= _interval) {
        _sendTime = millis();
        sendData();
    }

    if (millis() - _tickTime >= ACTIVE_INTERVAL_MS) {
        _flowMeter.tick(millis() - _tickTime);
        _tickTime = millis();
        if (abs(_flowMeter.getCurrentVolume() - _lastWaterCurrentVolume) > 0.001) {
            _lastWaterCurrentVolume = _flowMeter.getCurrentVolume();
            _sendTime = millis();
            sendData();            
        }
    }
}

void WaterFlowMetricsSender::sendData(void) {
    _sendCallbackFunction(
        getTopic(MQTT_TOPIC_WATER_FLOW_RATE_TPL), 
        String(_flowMeter.getCurrentFlowrate(), 2)
    );

    _sendCallbackFunction(
        getTopic(MQTT_TOPIC_WATER_TOTAL_VOLUME_TPL), 
        String(_flowMeter.getTotalVolume(), 2)
    );

    _sendCallbackFunction(
        getTopic(MQTT_TOPIC_WATER_VOLUME_TPL), 
        String(_flowMeter.getCurrentVolume(), 2)
    );
}