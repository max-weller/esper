#ifndef HLW8012_H
#define HLW8012_H

#include "Feature.h"


static unsigned int hlw8012_mode;

static uint32_t power_pulse_width;
static uint32_t current_pulse_width;
static uint32_t voltage_pulse_width;

static uint32_t last_cf_interrupt;
static uint32_t first_cf1_interrupt;
static uint32_t last_cf1_interrupt;

template<const char* const name, uint16_t gpio_sel, uint16_t gpio_cf, uint16_t gpio_cf1>
class HLW8012 : public Feature<name> {

public:

    HLW8012(Device* device) : Feature<name>(device) {
        pinMode(gpio_cf, INPUT_PULLUP);
        pinMode(gpio_cf1, INPUT_PULLUP);
        pinMode(gpio_sel, OUTPUT);

        // initialize to current mode
        hlw8012_mode = MODE_CURRENT;
        digitalWrite(gpio_sel, hlw8012_mode);

        this->registerProperty("voltage", NodeProperty(NULL, PropertyDataType::Integer, "Voltage", "", "V"));
        this->registerProperty("current", NodeProperty(NULL, PropertyDataType::Integer, "Current", "", "A"));
        this->registerProperty("power_active", NodeProperty(NULL, PropertyDataType::Integer, "Active power", "", "W"));
        this->registerProperty("power_apparent", NodeProperty(NULL, PropertyDataType::Integer, "Apparent power", "", "VA"));
        this->registerProperty("power_reactive", NodeProperty(NULL, PropertyDataType::Integer, "Reactive power", "", "var"));

        attachInterrupt(gpio_cf, &HLW8012::cfInterrupt, FALLING);
        attachInterrupt(gpio_cf1, &HLW8012::cf1Interrupt, FALLING);

        this->republish.initializeMs(5000, TimerDelegate(&HLW8012::publishCurrentState, this));
        this->republish.start();
        debugf("timer set, publishing every 5000ms");
    }

protected:
    void registerSubscriptions() {};
    void publishCurrentState() {
        debugf("publishing state mode=%d", hlw8012_mode);

        this->publish("voltage", String(this->getVoltage()), false);
        this->publish("current", String(this->getCurrent()), false);
        this->publish("power_active", String(this->getActivePower()), false);
        this->publish("power_apparent", String(this->getApparentPower()), false);
        this->publish("power_reactive", String(this->getReactivePower()), false);
    };

private:

    enum hlw8012_mode_t {
        MODE_VOLTAGE = 0,
        MODE_CURRENT = 1
    };

    Timer republish;

    unsigned int voltage;
    double current;

    const double voltage_multiplier = 1944.0;
    const double current_multiplier = 3500.0;
    const double power_multiplier = 12530.0;

    const double voltage_reference = 2300; // 230V
    const double current_reference = 4350; // 4.35A
    const double power_reference = 10000; // 1000W

    static const unsigned int pulse_timeout = 2000000;

    static IRAM_ATTR void toggleMode() {
        hlw8012_mode = 1 - hlw8012_mode;

        // pasted digitalWrite code here so no FLASH method has to be called...
        GPIO_REG_WRITE((((hlw8012_mode != LOW) ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS)), (1<<gpio_sel));
        debugf("mode switched to %d", hlw8012_mode);
        last_cf1_interrupt = first_cf1_interrupt = system_get_time();
    }

    static IRAM_ATTR void cfInterrupt() {
        const uint32_t now = system_get_time();
        power_pulse_width = now - last_cf_interrupt;
        last_cf_interrupt = now;
    };

    static IRAM_ATTR void cf1Interrupt() {
        const uint32_t now = system_get_time();
        uint32_t pulse_width;

        if ((now - first_cf1_interrupt) > pulse_timeout) {
            // measure pulse width
            if (last_cf1_interrupt == first_cf1_interrupt) {
                pulse_width = 0;
            } else {
                pulse_width = now - last_cf1_interrupt;
            }

            // assign pulse width depending on the mode
            if (hlw8012_mode == MODE_CURRENT) {
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
        debugf("getCurrent powerpulse=%d currentpulse=%d", power_pulse_width, current_pulse_width);
        if (power_pulse_width == 0) {
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
        if ((system_get_time() - last_cf1_interrupt) > pulse_timeout) {
            if (hlw8012_mode == MODE_CURRENT) {
                current_pulse_width = 0;
            } else {
                voltage_pulse_width = 0;
            }

            toggleMode();
        }
    }
};

#endif