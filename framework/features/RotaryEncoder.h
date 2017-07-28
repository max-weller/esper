#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H

#include "Feature.h"


template<const char* const name, uint16_t gpio_a, uint16_t gpio_b>
class RotaryEncoder : public Feature<name> {

protected:

public:
    using Callback = Delegate<void(bool up)>;

    RotaryEncoder(Device* device,
           const Callback& callback) :
            Feature<name>(device),
            callback(callback) {

        attachInterrupt(gpio_a, Delegate<void()>(&RotaryEncoder::onInterrupt, this), CHANGE);
        attachInterrupt(gpio_b, Delegate<void()>(&RotaryEncoder::onInterrupt, this), CHANGE);
        pinMode(gpio_a, INPUT_PULLUP);
        pinMode(gpio_b, INPUT_PULLUP);
    }

protected:
    virtual void publishCurrentState() {
    }

    virtual void registerSubscriptions() {
    }

private:
    virtual void onInterrupt()  {

        // Handle Rotary Encoder logic
        static uint8_t old_AB = 3;  //lookup table index
        static int8_t encval = 0;   //encoder value  
        const int8_t enc_states [] = 
            {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  //encoder lookup table
        /**/
        old_AB <<=2;  //remember previous state
        old_AB |= (digitalRead(gpio_a)<<1) | digitalRead(gpio_b);
        encval += enc_states[( old_AB & 0x0f )];
        //debugf("intr %02x, %d", old_AB, encval);

        if (encval>3) {
            encval=0;
            callback(true);
            this->publish("action", "+");
        }
        if (encval<-3) {
            encval=0;
            callback(false);
            this->publish("action", "-");
        }
    }
    
    const Callback callback;
};


#endif
