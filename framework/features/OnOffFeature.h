#ifndef ONOFF_H
#define ONOFF_H

#include "Feature.h"


template<const char* const name, uint16_t gpio, bool invert = false, uint16_t damper = 0>
class OnOffFeature : public Feature<name> {
    constexpr static const char* const ON = "1";
    constexpr static const char* const OFF = "0";

protected:

public:
    OnOffFeature(Device* const device, bool initial_state = false) :
            Feature<name>(device),
            state(initial_state),
            lastChange(RTC.getRtcNanoseconds()) {
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, this->state == !invert);

        this->registerSubscription("set", Device::MessageCallback(&OnOffFeature::onMessageReceived, this));

        debug_d("Initialized");
    }

    void set(bool state) {
        if (this->state = state) {
            debug_d("Turning on");
        } else {
            debug_d("Turning off");
        }

        digitalWrite(gpio, state == !invert);

        this->publishCurrentState();
    }

protected:
    virtual void publishCurrentState() {
        this->publish("", this->state ? ON : OFF, true);
    }

private:
    void onMessageReceived(const String& topic, const String& message) {
        const uint64_t now = RTC.getRtcNanoseconds();

        if (damper > 0) {
            if (this->lastChange + (damper * NS_PER_SECOND) > now)
                return;
        }

        if (message == ON) {
            this->set(true);

        } else if (message == OFF) {
            this->set(false);

        } else {
            debug_d("Unknown message received: %s", message.c_str());
        }

        this->lastChange = now;
    }

    bool state;

    uint64_t lastChange;
};

#endif
