#ifndef MQTTCONNECTION_MANAGER_H
#define MQTTCONNECTION_MANAGER_H

#include <SmingCore/SmingCore.h>

extern const char MQTT_CONNECTION_MANAGER_LOG_NAME[];
class MqttConnectionManager : public Log<MQTT_CONNECTION_MANAGER_LOG_NAME> {
public:
	enum class MqttState {
		CONNECTED,
		CONNECTING,
		DISCONNECTED
	};
	typedef Delegate<void(MqttState)> MqttConnectionStateChangedDelegate;

private:
	MqttState state;
	MqttClient client;
	MqttConnectionStateChangedDelegate stateCallback;
	MqttStringSubscriptionCallback messageCallback;
	bool started;
	Timer reconnectTimer;
public:
	MqttConnectionManager(MqttConnectionStateChangedDelegate cb, MqttStringSubscriptionCallback messageCallback) :
		state(MqttState::DISCONNECTED),
		client(MQTT_HOST, MQTT_PORT, MqttStringSubscriptionCallback(&MqttConnectionManager::onMessageReceived, this)),
		stateCallback(cb),
		messageCallback(messageCallback),
		started(false)
	{}

	inline MqttState getState() const {
		return state;
	}

	void connect() {
		log("Connecting");
		reconnectTimer.stop();
		setState(MqttState::CONNECTING);
		client.connect(WifiStation.getMAC());
		client.setCompleteDelegate(TcpClientCompleteDelegate(&MqttConnectionManager::onDisconnected, this));
		setState(MqttState::CONNECTED);
	}

	void subscribe(const String& topic) {
		client.subscribe(topic);
	}

	inline void publish(const String& topic, const String& message) {
		client.publish(topic, message);
	}
private:


	void setState(MqttState state) {
		this->state = state;
		stateCallback(state);
	}

	void onDisconnected(TcpClient &client, bool flag) {
		if (flag) {
			log("Disconnected.");
		} else {
			log("Unreachable.");
		}
		setState(MqttState::DISCONNECTED);
		reconnectTimer.initializeMs(2000, TimerDelegate(&MqttConnectionManager::connect, this)).start();
	}

	void onMessageReceived(const String topic, const String message) {
		log("FIXME, topic received: ", topic);	
		log("FIXME, message received: ", message);
	}
};

#endif