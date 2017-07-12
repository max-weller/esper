#include "MqttConnectionManager.h"


MqttConnectionManager::MqttConnectionManager(const StateChangedCallback& stateChangedCallback,
                                             const MessageCallback& messageCallback) :
        state(State::DISCONNECTED, stateChangedCallback),
        client(MQTT_HOST, MQTT_PORT,
               MqttStringSubscriptionCallback(&MqttConnectionManager::onMessageReceived, this)),
        messageCallback(messageCallback) {
    this->reconnectTimer.initializeMs(2000, TimerDelegate(&MqttConnectionManager::connect, this));

    debug_d("Initialized");
}

MqttConnectionManager::~MqttConnectionManager() {
}

void MqttConnectionManager::connect() {
    debug_d("Connecting");

    this->reconnectTimer.stop();
    this->state.set(State::CONNECTING);

    this->client.setCompleteDelegate(TcpClientCompleteDelegate(&MqttConnectionManager::onDisconnected, this));
    if (this->client.connect(WifiStation.getMAC())) {
        this->state.set(State::CONNECTED);

    } else {
        this->state.set(State::DISCONNECTED);

        debug_d("Failed to connect - reconnecting");
        this->reconnectTimer.start();
    }
}

void MqttConnectionManager::subscribe(const String &topic) {
    this->client.subscribe(topic);
    debug_d("Subscribed to: %s", topic.c_str());
}

void MqttConnectionManager::publish(const String &topic, const String &message, const bool& retain) {
    debug_d("Publishing message to %s:%s", topic.c_str(), message.c_str());
    this->client.publish(topic, message, retain);
}

MqttConnectionManager::State MqttConnectionManager::getState() const {
    return this->state;
}

void MqttConnectionManager::onDisconnected(TcpClient &client, bool flag) {
    if (flag) {
        debug_d("Disconnected");
    } else {
        debug_d("Unreachable");
    }

    this->state.set(State::DISCONNECTED);
    this->reconnectTimer.start();
}

void MqttConnectionManager::onMessageReceived(const String topic, const String message) {
    debug_d("Received topic: %s", topic.c_str());
    debug_d("Received message: %s", message.c_str());

    this->messageCallback(topic, message);
}
