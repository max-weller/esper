#ifndef HLW8012_H
#define HLW8012_H

#include "Feature.h"



template<const char* const name, uint16_t gpio_sel, uint16_t gpio_cf, uint16_t gpio_cf1>
class HLW8012 : public Feature<name> {

public:

    HLW8012(Device* device) : Feature<name>(device) {
        pinMode(gpio_cf, INPUT_PULLUP);
        pinMode(gpio_cf1, INPUT_PULLUP);
        pinMode(gpio_sel, OUTPUT);

        // initialize to current mode
        mode = MODE_CURRENT;
        digitalWrite(gpio_sel, mode);

        attachInterrupt(gpio_cf, Delegate<void()>(&HLW8012::cfInterrupt, this), FALLING);
        attachInterrupt(gpio_cf1, Delegate<void()>(&HLW8012::cf1Interrupt, this), FALLING);

        this->republish.initializeMs(5000, TimerDelegate(&HLW8012::publishCurrentState, this));
        this->republish.start();
        debugf("timer set, publishing every 5000ms");
    }

protected:
    void registerSubscriptions() {};
    void publishCurrentState() {
        debugf("publishing state");

        this->publish("voltage", String(this->getVoltage()), false);
        this->publish("current", String(this->getCurrent()), false);
        this->publish("power_active", String(this->getActivePower()), false);
        this->publish("power_apparent", String(this->getApparentPower()), false);
        this->publish("power_reactive", String(this->getReactivePower()), false);
    };

private:

    enum mode_t {
        MODE_VOLTAGE = 0,
        MODE_CURRENT = 1
    };

    Timer republish;

    unsigned int mode;

    unsigned int power;
    unsigned int voltage;
    double current;

    unsigned long power_pulse_width;
    unsigned long current_pulse_width;
    unsigned long voltage_pulse_width;

    unsigned long last_cf_interrupt;
    unsigned long first_cf1_interrupt;
    unsigned long last_cf1_interrupt;

    const double voltage_multiplier = 1944.0;
    const double current_multiplier = 3500.0;
    const double power_multiplier = 12530.0;

    const double voltage_reference = 2300; // 230V
    const double current_reference = 4350; // 4.35A
    const double power_reference = 10000; // 1000W

    const unsigned int pulse_timeout = 2000000;

    void toggleMode() {
        mode = 1 - mode;
        digitalWrite(gpio_sel, mode);
        debugf("mode switched to %d", mode);
        last_cf1_interrupt = first_cf1_interrupt = micros();
    }

    void cfInterrupt() {
        const unsigned long now = micros();
        power_pulse_width = now - last_cf_interrupt;
        last_cf_interrupt = now;
    };

    void cf1Interrupt() {
        const unsigned long now = micros();
        unsigned long pulse_width;

        if ((now - first_cf1_interrupt) > pulse_timeout) {
            // measure pulse width
            if (last_cf1_interrupt == first_cf1_interrupt) {
                pulse_width = 0;
            } else {
                pulse_width = now - last_cf1_interrupt;
            }

            // assign pulse width depending on the mode
            if (mode == MODE_CURRENT) {
                current_pulse_width = pulse_width;
            } else {
                voltage_pulse_width = pulse_width;
            }

            // switch mode
            toggleMode();

            // save state for next measurement
            first_cf1_interrupt = now;
        }

        last_cf1_interrupt = now;
    }

    double getCurrent() {
        //this->publish("debug/current/pulse_width", String(current_pulse_width), false);
        if (power == 0) {
            // power has a dedicated interrupt and therefore a snappier reaction
            // per P=U*I, if P=0, I must be 0 as well.
            current_pulse_width = 0;
        }

        checkCF1();

        if (current_pulse_width > 0) {
            return (current_reference * current_multiplier) / current_pulse_width / 1000.0f;
        };

        return 0.0f;
    }

    unsigned int getVoltage() {
        //this->publish("debug/voltage/pulse_width", String(voltage_pulse_width), false);
        checkCF1();

        if (voltage_pulse_width > 0) {
            return (voltage_reference * voltage_multiplier) / voltage_pulse_width / 10.0f;
        }

        return 0.0f;
    }

    unsigned int getActivePower() {
        if (power_pulse_width > 0) {
            return (power_reference * power_multiplier) / power_pulse_width / 10.0f;
        }

        return 0;
    }

    unsigned int getApparentPower() {
        double current = getCurrent();
        unsigned int voltage = getVoltage();
        return current * voltage;
    }

    unsigned int getReactivePower() {
        unsigned int active_power = getActivePower();
        unsigned int apparent_power = getApparentPower();

        if (apparent_power > active_power) {
            return sqrt((apparent_power * apparent_power) - (active_power * active_power));
        }

        return 0;
    }

    double getPowerFactor() {
        unsigned int active_power = getActivePower();
        unsigned int apparent_power = getApparentPower();

        if (active_power > apparent_power) {
            return 1;
        } else if (apparent_power == 0) {
            return 0;
        } else {
            return (double) active_power / apparent_power;
        }
    }

    void checkCF() {
        if ((micros() - last_cf_interrupt) > pulse_timeout) {
            power_pulse_width = 0;
        }
    }

    void checkCF1() {
        if ((micros() - last_cf1_interrupt) > pulse_timeout) {
            if (mode == MODE_CURRENT) {
                current_pulse_width = 0;
            } else {
                voltage_pulse_width = 0;
            }

            toggleMode();
        }
    }
};

#endif