#include "Heartbeat.h"
#include "../Device.h"


const char HEARTBEAT_NAME[] = "heartbeat";

Heartbeat::Heartbeat(Device* const device)
        : Service(device) {
    // Receive heartbeat messages
    this->device->registerProperty(HEARTBEAT_TOPIC, NodeProperty(Device::MessageCallback(&Heartbeat::onMessageReceived, this), PropertyDataType::Command, ""));

    // Reboot the system if heartbeat was missing
    this->timer.initializeMs(120000, TimerDelegate(&Device::reboot, device));
}

Heartbeat::~Heartbeat() {
}

void Heartbeat::onStateChanged(const State& state) {
    switch (state) {
        case State::CONNECTED: {
            debug_d("Start awaiting heartbeats");
            this->timer.start();

            break;
        }

        case State::DISCONNECTED: {
            // Heartbeats are likely to miss if disconnected
            debug_d("Stop awaiting heartbeats");
            this->timer.stop();

            break;
        }
    }
}

void Heartbeat::onMessageReceived(const String& topic, const String& message) {
    // Handle incoming heartbeat
    debug_d("Heartbeat");
    this->timer.restart();
}
