#ifndef BUTTON_H
#define BUTTON_H

#include "Feature.h"
#include "../util/Observed.h"


template<const char* const name, uint16_t gpio, bool inverted>
class Button : public Feature<name> {
    constexpr static const char* const ON = "1";
    constexpr static const char* const OFF = "0";

protected:

public:
    using Callback = Observed<bool>::Callback;

    Button(Device* const device,
           const Callback& callback) :
            Feature<name>(device),
            state(false, callback) {
        attachInterrupt(gpio, Delegate<void()>(&Button::onInterrupt, this), CHANGE);
    }

protected:
    virtual void publishCurrentState() {
        debug_d("Current state: %d", (bool)this->state);

        this->publish("", this->state ? ON : OFF, true);
    }

    virtual bool onEdge(const bool& edge) = 0;

private:
    virtual void onInterrupt()  {
        const bool state = this->onEdge(digitalRead(gpio) == !inverted);

        debug_d("Old state: %d", (bool)this->state);
        debug_d("New state: %d", state);

        if (state != this->state) {
            this->state.set(state);
            this->publishCurrentState();
        }
    }

    Observed<bool> state;
};


#endif
