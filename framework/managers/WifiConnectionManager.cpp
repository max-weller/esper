#include "WifiConnectionManager.h"



WifiConnectionManager::WifiConnectionManager(const StateChangedCallback& callback) :
        state(State::DISCONNECTED, callback) {
    this->reconnectTimer.initializeMs(2000, TimerDelegate(&WifiConnectionManager::connect, this));

    debug_d("Initalized");
}

WifiConnectionManager::~WifiConnectionManager() {
}

void WifiConnectionManager::connect() {
    debug_d("Connecting");

    this->reconnectTimer.stop();

    debug_d("SSID: %s", WIFI_SSID);
    debug_d("PW: %s", WIFI_PWD);
    WifiStation.config(WIFI_SSID, WIFI_PWD);
    debug_d("Configured");

    WifiAccessPoint.enable(false);
    debug_d("Wifi Access Point disabled");

    WifiStation.enable(true);
    debug_d("Wifi Station enabled");

    debug_d("Waiting for connection");
    WifiStation.waitConnection(ConnectionDelegate(&WifiConnectionManager::onConnectOk, this), 10,
                               ConnectionDelegate(&WifiConnectionManager::onConnectFail, this));

    this->state.set(State::CONNECTING);
}

WifiConnectionManager::State WifiConnectionManager::getState() const {
    return this->state;
}


void WifiConnectionManager::onConnectOk() {
    debug_d("Connected");

    if (this->state.set(State::CONNECTED)) {
        this->reconnectTimer.stop();
    }
}

void WifiConnectionManager::onConnectFail() {
    debug_d("Disconnected");

    if (this->state.set(State::DISCONNECTED)) {
        this->reconnectTimer.start();
    }
}
