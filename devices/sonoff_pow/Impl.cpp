#include "../features/Socket.h"
#include "../features/ToggleButton.h"
#include "../features/Light.h"
#include "../features/Hlw8012.h"
#include "Device.h"


constexpr const char SOCKET_NAME[] = "socket";
constexpr const uint16_t SOCKET_GPIO = 12;

constexpr const char BUTTON_NAME[] = "button";
constexpr const uint16_t BUTTON_GPIO = 0;

constexpr const char LIGHT_NAME[] = "light";
constexpr const uint16_t LIGHT_GPIO = 15;

constexpr const char METER_NAME[] = "meter";
constexpr const uint16_t METER_SEL_GPIO = 5;
constexpr const uint16_t METER_CF_GPIO = 14;
constexpr const uint16_t METER_CF1_GPIO = 13;


class SonoffPowDevice : public Device {
public:
    SonoffPowDevice() :
            socket(this),
            light(this),
            meter(this),
            button(this, decltype(button)::Callback(&SonoffPowDevice::onStateChanged, this)) {
        this->add(&(this->socket));
        this->add(&(this->light));
        this->add(&(this->button));
        this->add(&(this->meter));
    }

private:
    void onStateChanged(const bool& state) {
        this->socket.set(state);
        this->light.set(state);
    }

    OnOffFeature<SOCKET_NAME, SOCKET_GPIO, false, false> socket;
    OnOffFeature<LIGHT_NAME, LIGHT_GPIO, false> light;
    ToggleButton<BUTTON_NAME, BUTTON_GPIO, true> button;
    HLW8012<METER_NAME, METER_SEL_GPIO, METER_CF_GPIO, METER_CF1_GPIO> meter;
};


Device* createDevice() {
    return new SonoffPowDevice();
}