#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include "Button.h"


template<const char* const name, uint16_t gpio, bool inverted = true, bool activeEdge = true>
class ToggleButton : public Button<name, gpio, inverted> {
protected:

public:
    using Callback = typename Button<name, gpio, inverted>::Callback;

    ToggleButton(Device* const device,
                 const Callback& callback) :
            Button<name, gpio, inverted>(device, callback),
            state(false) {
    }

protected:
    virtual bool onEdge(const bool& edge) {
        debug_d("Edge: %d", edge);

        if (edge == activeEdge) {
            this->state = !this->state;
        }

        debug_d("State: %d", this->state);
        return this->state;
    }

private:
    bool state;
};


#endif
