#include "WifiConnectionManager.h"
#include "util/Strings.h"



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

    this->state.set(State::CONNECTING);

    debug_d("Waiting for connection");
    WifiEvents.onStationGotIP(StationGotIPDelegate(&WifiConnectionManager::onStationConfigured, this));
    WifiEvents.onStationConnect(StationConnectDelegate(&WifiConnectionManager::onStationConnected, this));
    WifiEvents.onStationDisconnect(StationDisconnectDelegate(&WifiConnectionManager::onStationDisconnected, this));
}

WifiConnectionManager::State WifiConnectionManager::getState() const {
    return this->state;
}

const String& WifiConnectionManager::getCurrentSSID() const {
    return this->ssid;
}

const String& WifiConnectionManager::getCurrentBSSID() const {
    return this->bssid;
}

void WifiConnectionManager::onStationConfigured(IPAddress ip, IPAddress mask, IPAddress gateway) {
    debug_d("Configured: ip=%s, mask=%s, gw=%s", String(ip).c_str(), String(mask).c_str(), String(gateway).c_str());

    if (this->state.set(State::CONNECTED)) {
        this->reconnectTimer.stop();
    }
}

void WifiConnectionManager::onStationConnected(String ssid, uint8_t, uint8_t bssid[6], uint8_t reason) {
    debug_d("Connected");

    this->ssid = ssid;
    this->bssid = Strings::formatMAC(bssid);
}

void WifiConnectionManager::onStationDisconnected(String, uint8_t, uint8_t[6], uint8_t reason) {
    debug_d("Disconnected");

    this->ssid = String();
    this->bssid = String();

    if (this->state.set(State::DISCONNECTED)) {
        this->reconnectTimer.start();
    }
}
