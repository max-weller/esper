#ifndef WIFI_CONNECTION_MANAGER_H
#define WIFI_CONNECTION_MANAGER_H

#include <SmingCore/SmingCore.h>

#include "../util/Observed.h"


class WifiConnectionManager {

    WifiConnectionManager(WifiConnectionManager const&) = delete;
    WifiConnectionManager& operator=(WifiConnectionManager const&) = delete;

public:
    enum class State {
        CONNECTED,
        CONNECTING,
        DISCONNECTED
    };

    using StateChangedCallback = Observed<State>::Callback;

public:
    WifiConnectionManager(const StateChangedCallback& callback);
    ~WifiConnectionManager();

    void connect();

    State getState() const;

private:
    void onConnectOk();
    void onConnectFail();

    Observed<State> state;
    Timer reconnectTimer;
};

#endif
